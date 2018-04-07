#include <Arduino.h>
#include "Dictionary.h"

// todo: handle error condition when prefs are too big

// entries are saved:
//   tag string (zero terminated), length byte, data bytes (of length given before)

bool Dictionary::write(tag_t tag, size_t size, const uint8_t* data) {

  bool changed = false;
  size_t offset = findTag(tag);

  if (offset) {
    // found an old tag
    size_t oldDataSize = tagDataSize(offset);
    if (oldDataSize == size) {
      // same size so we can write in place
      writeTag(offset, tag, size, data);
      changed = true;
    } else {
      if (((int)remaining() - oldDataSize + size) < 0) {
        // not enough room to write
        return false;
      }

      // delete pref if the new size is not the same as the existing one
      deleteTag(offset);
      offset = 0;
      changed = true;
    }
  } else {
    if (((int)remaining() - size) < 0) {
      // not enough room to write
      return false;
    }
  }
  // if it isn't already written and it has a size, we'll tack it on to the end
  if (!offset && size) {
    // append to the end
    offset = findTag(endTag);
    offset = writeTag(offset, tag, size, data);
    offset = writeTag(offset, endTag, sizeof(version), &version);
    changed = true;
  }

  // save it out if something's been written
  if (changed) {
    save();
  }
  return true;
}

size_t Dictionary::size(tag_t tag) {
  size_t offset = findTag(tag);

  if (offset) {
    return tagDataSize(offset);
  } else {
    return 0;  // not found
  }
}

size_t Dictionary::read(tag_t tag, size_t size, uint8_t* data) {

  size_t offset = findTag(tag);

  if (offset) {
    offset += strlen(tag) + 1;
    size_t readSize = prefsData[offset];
    offset++;
    readSize = min(readSize, size);  // if the sizes don't match, choose the smaller
    memcpy(data, &prefsData[offset], readSize);

    return readSize;
  } else {
    return 0;  // not found
  }
}

bool Dictionary::write(String& tag, String& data) {
  return write(tag.c_str(), data.length()+1, (const uint8_t*)data.c_str());
}

size_t Dictionary::read(String& tag, String& data) {
  size_t offset = findTag(tag.c_str());

  if (offset) {
    offset += tag.length() + 1;
    offset++;
    data = (const char*) (&prefsData[offset]);

    return data.length();
  } else {
    data = "";
    return 0;  // not found
  }
}


void Dictionary::deleteTag(size_t start) {
  // delete chosen tag and close the gap

  // don't delete the version tag
  if (start == 0) {
    return;
  }

  // close the gap
  size_t end = findTag(endTag);

  size_t nextTag = tagSize(start) + start;

  size_t len = end - nextTag + tagSize(end);

  if (nextTag <= end) {
    memmove(prefsData+start, prefsData+nextTag, len);
  }

}

size_t Dictionary::tagSize(size_t offset) {
  size_t endoffset = offset;
  endoffset += strlen((const char*)&prefsData[endoffset]) + 1;
  endoffset += prefsData[endoffset];
  endoffset++;

  return endoffset - offset;
}

size_t Dictionary::tagDataSize(size_t offset) {
  size_t endoffset = offset;
  endoffset += strlen((const char*)&prefsData[endoffset]) + 1;
  return prefsData[endoffset];
}

size_t Dictionary::findTag(tag_t tag) {
  size_t offset = 0;
  while (offset < totalPrefsSize) {
    if (strcmp(tag, (const char*)&prefsData[offset]) == 0) {
      return offset;
    } else if (strcmp(endTag, (const char*)&prefsData[offset]) == 0) {
      return 0;  // hit the end tag, so it's not found
    } else {
      offset += strlen((const char*)&prefsData[offset]) + 1;
      offset += prefsData[offset] + 1;
    }
  }

  // went past the end of the data structure, we are corrupted.
  reset();
  return 0;
}

size_t Dictionary::used() {
  size_t offset = findTag(endTag);
  return offset + tagSize(offset);
}

size_t Dictionary::writeTag(size_t offset, tag_t tag, size_t size, const uint8_t* data) {
    if (size > MAX_TAG_SIZE) {
      return 0;
    }
    strcpy((char*)&prefsData[offset], tag);
    offset += strlen(tag) + 1;

    prefsData[offset] = size;
    offset++;

    memcpy(&prefsData[offset], data, size);
    offset+= size;

    return offset;
}

void EEPROMDictionary::reset() {
    // initial version tag is missing, clear out prefs
    size_t offset = 0;
    // write a version tag & 1 byte version
    offset = writeTag(offset, versionTag, sizeof(version), &version);

    // write an end tag with the same 1 byte version
    offset = writeTag(offset, endTag, sizeof(version), &version);
    save();
}

void EEPROMDictionary::save() {
  // this is brute force:
  // todo - only write out the bytes that have changed
  for (size_t i = 0; i < totalPrefsSize; i++) {
    EEPROM.write(i, prefsData[i]);
  }
}

void EEPROMDictionary::load(uint8_t initVersion) {
  version = initVersion;

  // todo - only read in the bytes that have been used
  for (size_t i = 0; i < totalPrefsSize; i++) {
    prefsData[i] = EEPROM.read(i);
  }

  // check if prefs have been initialized
  if (strcmp((const char*)versionTag, (const char*)prefsData) != 0 ||
      prefsData[strlen(versionTag) + 1] != sizeof(version) ||
      prefsData[strlen(versionTag) + 2] != version) {
    reset();
  }
}

