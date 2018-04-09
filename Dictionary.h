#ifndef _Dictionary_
#define _Dictionary_

#include "EEPROM.h"
#include "WString.h"

typedef const char* tag_t;

class Dictionary {
  public:
    ~Dictionary();
    virtual void begin(uint8_t initVersion = 1);

    bool set(tag_t tag, size_t size, const uint8_t* data);  // return true on success
    bool set(String& tag, String& data);

    size_t get(tag_t tag, size_t size, uint8_t* data);
    size_t get(String& tag, String& data);

    void reset();  // danger: this erases all the data

    uint8_t getVersion() { return version; }

    size_t size(tag_t tag); // size of a given tag, returns 0 if it doesn't exist
    size_t size() { return dictDataSize; }  // returns total size of dict memory allocations
    size_t used();  // returns total amount of space used

    size_t remaining() { return size() - used(); }

    static constexpr size_t MAX_ENTRY_SIZE = 255;

    const uint8_t* data() { return dictData; }  // for debugging only

  protected:
    size_t dictDataSize = 0;
    uint8_t* dictData = nullptr;
    uint8_t version;

    virtual void save() {};
    virtual bool resize(size_t newSize);  // returns false if resizing fails

    size_t findTag(tag_t tag);
    size_t writeTag(size_t offset, tag_t tag, size_t size, const uint8_t* data);
    size_t tagDataSize(size_t offset);
    void deleteTag(size_t start);
    size_t tagSize(size_t offset);

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
    virtual void begin(uint8_t initVersion = 1);  // 1 - default version

  protected:
    virtual bool resize(size_t newSize) { return false; }  // we can't resize the flash area
    virtual void save();

  private:
    uint8_t EEPromData[EEPROM_SIZE];  // this keeps a full copy of the flash in RAM, which is wasteful, but avoids complications of using FlexRAM directly
};

#endif
