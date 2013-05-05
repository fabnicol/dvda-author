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
        commandLineDepthMask=0xF,
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
};
#endif // ENUMS_H
