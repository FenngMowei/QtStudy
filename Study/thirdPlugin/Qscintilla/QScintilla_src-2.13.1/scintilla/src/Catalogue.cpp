// Scintilla source code edit control
/** @file Catalogue.cxx
 ** Lexer infrastructure.
 ** Contains a list of LexerModules which can be searched to find a module appropriate for a
 ** particular language.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <vector>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "LexerModule.h"
#include "Catalogue.h"

using namespace Scintilla;

static std::vector<LexerModule *> lexerCatalogue;
static int nextLanguage = SCLEX_AUTOMATIC+1;

const LexerModule *Catalogue::Find(int language) {
    Scintilla_LinkLexers();
    for (const LexerModule *lm : lexerCatalogue) {
        if (lm->GetLanguage() == language) {
            return lm;
        }
    }
    return nullptr;
}

const LexerModule *Catalogue::Find(const char *languageName) {
    Scintilla_LinkLexers();
    if (languageName) {
        for (const LexerModule *lm : lexerCatalogue) {
            if (lm->languageName && (0 == strcmp(lm->languageName, languageName))) {
                return lm;
            }
        }
    }
    return nullptr;
}

void Catalogue::AddLexerModule(LexerModule *plm) {
    if (plm->GetLanguage() == SCLEX_AUTOMATIC) {
        plm->language = nextLanguage;
        nextLanguage++;
    }
    lexerCatalogue.push_back(plm);
}

// To add or remove a lexer, add or remove its file and run LexGen.py.
// qiucx: register the lexers
// Force a reference to all of the Scintilla lexers so that the linker will
// not remove the code of the lexers.
int Scintilla_LinkLexers() {

    static int initialised = 0;
    if (initialised)
        return 0;
    initialised = 1;

// Shorten the code that declares a lexer and ensures it is linked in by calling a method.
#define LINK_LEXER(lexer) extern LexerModule lexer; Catalogue::AddLexerModule(&lexer);

//++Autogenerated -- run scripts/LexGen.py to regenerate
//**\(\tLINK_LEXER(\*);\n\)
    //LINK_LEXER(lmCmake);
    //LINK_LEXER(lmCPP);
    //LINK_LEXER(lmMatlab);
    LINK_LEXER(lmModelica); // qiucx: add new lexer:lmModelica
//--Autogenerated -- end of automatically generated section

    return 1;
}
