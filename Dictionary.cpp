#include <Arduino.h>
#include "Dictionary.h"

typedef uint8_t datasize_t;

Dictionary::~Dictionary() {
  free(dictData);
}

Dictionary::Dictionary() {
}

// entries are saved:
//   tag string (zero terminated), length byte, data bytes (of length given before)

bool Dictionary::set(tag_t tag, size_t size, const uint8_t* data) {

  if (size > MAX_ENTRY_SIZE) { return false; }

// initialize empty dict with end tag
  if (dictData == nullptr) {
    size_t endTagSize = strlen(endTag)+1+sizeof(datasize_t)+1;
    dictData = (uint8_t*)malloc(endTagSize);
    writeEndTag(0);
  }

  bool changed = false;
  bool append = false;
  size_t offset;

  if (findTag(tag, &offset)) {
    // found an old tag
    size_t oldDataSize = tagDataSize(offset);

    if (oldDataSize == size) {
      // same size so we can write in place
      writeTag(offset, tag, size, data);
      changed = true;
    } else {

      size_t newSize = used() - oldDataSize + size;
      // delete pref if the new size is not the same as the existing one
      deleteTag(offset);

      if (!resize(newSize)) {
        // not enough room to resize
        return false;
      }

      offset = 0;
      changed = true;
      append = true;
    }
  } else {
    // need space for tag + nul char + one byte of size info + the actual data
    size_t spaceNeeded = strlen(tag) + 1 + sizeof(datasize_t) + size;
    if (!resize(used() + spaceNeeded)) {
      // not enough room to write
      return false;
    }
    append = true;
  }

  // if it isn't already written and it has a size, we'll tack it on to the end
  if (append && size) {
    // append to the end
    findTag(endTag, &offset);
    offset = writeTag(offset, tag, size, data);
    writeEndTag(offset);
    changed = true;
  }

  // save it out if something's been written
  if (changed) {
    save();
  }
  return true;
}

bool Dictionary::set(String& tag, String& data) {
  return set(tag.c_str(), data.length()+1, (const uint8_t*)data.c_str());
}

bool Dictionary::set(String& tag) {
  return set(tag.c_str());
}

size_t Dictionary::size(tag_t tag) {
  size_t offset;

  if (findTag(tag,&offset)) {
    return tagDataSize(offset);
  } else {
    return 0;  // not found
  }
}

size_t Dictionary::get(tag_t tag, size_t size, uint8_t* data) {

  size_t offset;

  if (findTag(tag,&offset)) {
    offset += strlen(tag) + 1;
    size_t readSize = dictData[offset];
    offset++;
    readSize = min(readSize, size);  // if the sizes don't match, choose the smaller
    memcpy(data, &dictData[offset], readSize);

    return readSize;
  } else {
    return 0;  // not found
  }
}

size_t Dictionary::get(String& tag, String& data) {
  size_t offset;

  if (findTag(tag.c_str(),&offset)) {
    offset += tag.length() + 1;
    offset++;
    data = (const char*) (&dictData[offset]);

    return data.length();
  } else {
    data = "";
    return 0;  // not found
  }
}


void Dictionary::deleteTag(size_t start) {
  // delete chosen tag and close the gap

  // close the gap
  size_t end;
  findTag(endTag, &end);

  size_t nextTag = tagSize(start) + start;

  size_t len = end - nextTag + tagSize(end);

  if (nextTag <= end) {
    memmove(dictData+start, dictData+nextTag, len);
  }
}

size_t Dictionary::tagSize(size_t offset) {
  size_t endoffset = offset;
  if (dictData == nullptr) { return 0; }
  endoffset += strlen((const char*)&dictData[endoffset]) + 1;
  endoffset += dictData[endoffset];
  endoffset++;

  return endoffset - offset;
}


size_t Dictionary::tagDataSize(size_t offset) {
  size_t endoffset = offset;
  if (dictData == nullptr) { return 0; }
  endoffset += strlen((const char*)&dictData[endoffset]) + 1;
  return dictData[endoffset];
}

// returns offset of data for a given tag, returns true if found
bool Dictionary::findTag(tag_t tag, size_t* offset) {
  if (dictData == nullptr) { return false; }

  size_t o = 0;
  while (1) {
    if (strcmp(tag, (const char*)&dictData[o]) == 0) {
      *offset = o;
      return true;
    } else if (strcmp(endTag, (const char*)&dictData[o]) == 0) {
      return false;  // hit the end tag, so it's not found
    } else {
      o += strlen((const char*)&dictData[o]) + 1;
      o += dictData[o] + 1;
    }
  }

  // went past the end of the data structure, we are corrupted.
  reset();
  return false;
}

// total amount of dictData used
size_t Dictionary::used() {
  if (dictData == nullptr) { return 0; }
  size_t offset;
  findTag(endTag, &offset);
  return offset + tagSize(offset);
}

size_t Dictionary::writeTag(size_t offset, tag_t tag, size_t size, const uint8_t* data) {
    if (size > MAX_ENTRY_SIZE) {
      return 0;
    }
    strcpy((char*)&dictData[offset], tag);
    offset += strlen(tag) + 1;

    dictData[offset] = size;
    offset++;

    memcpy(&dictData[offset], data, size);
    offset+= size;

    return offset;
}

void Dictionary::reset() {
    // initial version tag is missing, clear out data
    free(dictData);
    dictData = nullptr;
}

bool Dictionary::resize(size_t newSize) {
  uint8_t* newData = (uint8_t*)realloc((void*)dictData, newSize);

  if (newData) {
    dictData = newData;
  }

  return newData != nullptr;
}

void Dictionary::writeEndTag(size_t offset) {
  uint8_t data = 0;
  writeTag(offset, endTag, sizeof(data), &data);
}

///////////////////////////////////////////////////////////////////////////////
// EEPROMDictionary

EEPROMDictionary::EEPROMDictionary() {
  dictData = EEPROMData;
}

void EEPROMDictionary::save() {
  // this is brute force:
  // todo - only write out the bytes that have changed
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, dictData[i]);
  }
}

void EEPROMDictionary::load() {
  // todo - only read in the bytes that have been used
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    dictData[i] = EEPROM.read(i);
  }

  // check if Dict have been initialized
  if (strcmp((const char*)versionTag, (const char*)dictData) != 0 ||
      dictData[strlen(versionTag) + 1] != sizeof(_version) ||
      dictData[strlen(versionTag) + 2] != _version) {
    reset();
  }
}

void EEPROMDictionary::reset() {
    // initial version tag is missing, clear out data
    size_t offset = 0;

    // write a version tag
    offset = writeTag(offset, versionTag, sizeof(_version), &_version);
    writeEndTag(offset);
    save();
}

