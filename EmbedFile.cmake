function (EmbedFile FILE HEADER VAR)
    file(READ ${FILE} DATA HEX)

    string(REGEX REPLACE "(..)" "0x\\1, " HEX_DATA ${DATA})

    set(EmbedFileContent "
#ifndef EMBED_FILE_${VAR}_H
#define EMBED_FILE_${VAR}_H

constexpr unsigned char _${VAR}Data[] = { ${HEX_DATA} }\;

#endif // EMBED_FILE_${VAR}_H
    ")

    file(WRITE ${HEADER} ${EmbedFileContent})
endfunction ()