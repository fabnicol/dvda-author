#ifndef ENUMS_H
#define ENUMS_H
#include <cstdint>

class flags
{
public:
    enum {flush=0xF00};
    enum {importFiles, importNames, typeIn, isEmbedded};

    enum class font : std::uint8_t {boldTitle, regularTitle, italicTitle};

    enum class commandLineType : std::uint8_t {dvdaCommandLine,
                                               defaultCommandLine=dvdaCommandLine,
                                               createDisc,
                                               createIso,
                                               dvdaExtract,
                                               lplexFiles,
                                               noCommandLine,
                                               commandLinewidgetDepthMask=0xF,
                                               commandLineMask=0xF,
                                              };

    enum class status : std::uint16_t {
        untoggledCommandLine=0x10,
        toggledCommandLine=0x20,
        enabled=0x100,
        disabled=0x200,
        checked=0x1000,
        unchecked=0x2000,
        multimodal=0x3000,
        enabledChecked=enabled|checked,
        enabledUnchecked=enabled|unchecked,
        hasListCommandLine=0x4000,
        defaultStatus=enabled,
        commandLineToggleMask=0xF0,
        enabledMask=0xF00,
        widgetMask=0xF000,
        statusMask=widgetMask|enabledMask|commandLineToggleMask  //0xFFF0
    };



    friend int operator | (int  x, flags::status y) {return x | static_cast<int>(y);}
    friend flags::status operator & (int  x, flags::status y) {return y & static_cast<flags::status>(x);}
    friend flags::status operator & (flags::status  x, flags::status y) {return static_cast<flags::status>(static_cast<int>(y) & static_cast<int>(x));}
    friend int operator | (flags::commandLineType x, flags::status y) {return static_cast<int>(x) | static_cast<int>(y);}
    friend int operator | (flags::status y, flags::commandLineType x) {return static_cast<int>(x) | static_cast<int>(y);}
    friend int operator & (flags::status y, flags::commandLineType x) {return static_cast<int>(x) & static_cast<int>(y);}
    friend int operator & (flags::commandLineType x, flags::status y) {return static_cast<int>(x) & static_cast<int>(y);}

    static int lplexRank;

};

enum actionType {Select, OpenFolder, BrowseFile};

enum  {
  hasUnsavedOptions=0x0,
  hasSavedOptions=0x1,
  hasSavedOptionsMask=0xF,
  UpdateTree=0x0010,
  UpdateTreeMask=0x00F0,
  SaveTree=0x0100,
  SaveTreeMask=0x0F00,
  UpdateMainTabs=0x1000,
  UpdateTabMask=0x7000,
  UpdateOptionTabs=0x2000,
  KeepOptionTabs=0x4000,
  ParseXml=0xF000,
  ParseXmlMask=0xF000
};

enum {
    refreshProjectManagerFlag=0x000,
    refreshProjectAudioZoneMask=0x00F,
    refreshProjectVideoZoneMask=0x0F0,
    refreshProjectSystemZoneMask=0xF00,
    refreshProjectInteractiveMask=0xF000,
    refreshAudioZone=0x001,
    refreshVideoZone=0x010,
    refreshSystemZone=0x100,
    refreshProjectInteractiveMode=0x1000,
    refreshAllZones=refreshAudioZone|refreshVideoZone|refreshSystemZone
};


#endif // ENUMS_H
