#ifndef ENUMS_H
#define ENUMS_H

class flags
{
public:
    enum {flush=0xF00};
    enum {importFiles, importNames, typeIn, isEmbedded};
    enum font {boldTitle, regularTitle, italicTitle};
    enum commandLineType {dvdaCommandLine, createDisc, createIso, dvdaExtract, lplexFiles, hasListCommandLine, noCommandLine};
    enum status {
        defaultStatus,
        defaultCommandLine,
        commandLineMask=0xF,
        commandLinewidgetDepthMask=0xF,
        untoggledCommandLine=0x00,
        toggledCommandLine=0x10,
        commandLineToggleMask=0xF0,
        enabled=0x000,
        disabled=0x100,
        enabledMask=0xF00,
        widgetMask=0xF000,
        checked=0x0000,
        unchecked=0x1000,
        multimodal=0x2000
    };
    static int lplexRank;

};

enum actionType {Select, OpenFolder, BrowseFile};

enum  {
  NoCreate=0x0000,
  CreateTree=0x0001,
  CreateTreeMask=0x000F,
  UpdateTree=0x0010,
  UpdateTreeMask=0x00F0,
  SaveTree=0x0100,
  SaveTreeMask=0x0F00,
  UpdateTabs=0x1000,
  UpdateTabMask=0xF000,
  UpdateOptionTabs=0x2000,
  ParseXml=0x3000,
  ParseXmlMask=0x3000
};


#endif // ENUMS_H
