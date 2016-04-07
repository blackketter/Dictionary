#ifndef _Preferences_
#define _Preferences_

#include "EEPROM.h"

typedef const char* tag_t;

class Preferences {
  public:
    Preferences(uint8_t initVersion = 1);  // 1 - default version

    void write(tag_t tag, uint8_t size, const uint8_t* data);
    uint8_t read(tag_t tag, uint8_t size, uint8_t* data);

    void resetPrefs();  // danger: this erases all the prefs

    uint8_t getVersion() { return version; }

  private:
    static const size_t totalPrefsSize = E2END+1;
    uint8_t prefsData[totalPrefsSize];  // this keeps a full copy of the flash in RAM, which is wasteful, but avoids complications of using FlexRAM directly

    void saveOut();

    uint8_t version;
    static constexpr tag_t versionTag = "vers";
    static constexpr tag_t endTag = "end";

    size_t writeTag(size_t offset, tag_t tag, uint8_t size, const uint8_t* data);
    size_t findTag(tag_t tag);
    size_t tagDataSize(size_t offset);
    void deleteTag(size_t start);
    size_t tagSize(size_t offset);

};

extern Preferences prefs;

#endif
