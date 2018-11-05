#ifndef _Dictionary_
#define _Dictionary_

#include "EEPROM.h"
#include "WString.h"

typedef const char* tag_t;

class Dictionary {
  public:
    ~Dictionary();
    Dictionary();

    bool set(tag_t tag, size_t size = 0, const uint8_t* data = nullptr);  // return true on success

    bool set(String& tag);
    bool set(String& tag, String& data);

    size_t get(tag_t tag, size_t size, uint8_t* data);
    size_t get(String& tag, String& data);

    virtual void reset();  // danger: this erases all the data


    size_t size(tag_t tag); // size of a given tag, returns 0 if it doesn't exist
    size_t used();  // returns total amount of space used

    static constexpr size_t MAX_ENTRY_SIZE = 255;

    const uint8_t* data() { return dictData; }  // for debugging only

  protected:
    uint8_t* dictData = nullptr;

    virtual void save() {};
    virtual bool resize(size_t newSize);  // returns false if resizing fails

    bool findTag(tag_t tag, size_t* offset);  // returns true on success
    size_t writeTag(size_t offset, tag_t tag, size_t size, const uint8_t* data);
    size_t tagDataSize(size_t offset);
    void deleteTag(size_t start);
    size_t tagSize(size_t offset);
    void writeEndTag(size_t offset);

    static constexpr tag_t versionTag = "vers";
    static constexpr tag_t endTag = "end";
};

#if TEENSY
#define EEPROM_SIZE (E2END+1)
#else
#define EEPROM_SIZE 4096
#endif

class EEPROMDictionary : public Dictionary {
  public:
    EEPROMDictionary();
    virtual void load();  // if the stored version doesn't match this version, then the data is reset
    virtual void reset();
    size_t remaining() { return EEPROM_SIZE - used(); }

  protected:
    virtual bool resize(size_t newSize) { return newSize <= EEPROM_SIZE; }
    virtual void save();
    const uint8_t _version = 1;

  private:
    uint8_t EEPROMData[EEPROM_SIZE];  // this keeps a full copy of the flash in RAM, which is wasteful, but avoids complications of using FlexRAM directly

};

#endif
