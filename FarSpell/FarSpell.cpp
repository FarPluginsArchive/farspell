/*
    Spell-checker plugin for FAR
    Copyright (c) Sergey Shishmintzev, Kiev 2005-2006

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define HARDCODED_MLDATA

#include <malloc.h>
#include <limits.h>
#include <stdio.h>
#define _WINCON_
#include <windows.h>
#undef _WINCON_
#pragma pack(push,8)
#include <wincon.h>
#pragma pack(pop)

#ifndef HARDCODED_MLDATA
#include <MLang.h>
#endif HARDCODED_MLDATA

#include "FARPlus/FARMemory.h"
#include "FARPlus/FARLog.h"
#include "FARPlus/FARPlus.h"
#include "FARPlus/FARRegistry.h"
#include "FARPlus/FARDialog.h"
#include "FARPlus/FAREd.h"
//#include "FARPlus/FARXml.h"
//#include "FARPlus/FARDbg.h"
#include "FARPlus/FARFile.h"

#define DIALOG_LINK
#include "DialogGenerator/API/Dialog.h"

//#include "hunspell/hunspell.h"
#include "hunspell/hunspell.hxx"
#define hunspell_free free
#include "parsers/textparser.hxx"
#include "parsers/textparser.hxx"
#include "parsers/htmlparser.hxx"
#include "parsers/latexparser.hxx"
#include "parsers/manparser.hxx"
#include "parsers/firstparser.hxx"

#ifdef HARDCODED_MLDATA
#include "codepages.cpp"
#endif

// Registry keys
char *const plugin_enabled_key = "PluginEnabled";
char *const editor_files_key = "EditorFiles";
char *const highlight_mask_key = "HighlightMask";
char *const highlight_color_key = "HighlightColor";
char *const highlight_deletecolor_key = "HighlightDeleteColor";
char *const suggestions_in_menu_key = "AreSuggesionsInMenu";
char *const enable_file_settings_key = "FileSettings";

// -- Language strings -------------------------------------------------------
enum {
  MDbgPleaseWait,
  MDbgContinue,
  MNoDbgMessages,
  MUnloadPlugin,
  MExitFAR,
  MDbgSeeMore,
  MExceptionIn,

  MError,
  MOk,
  MCancel,

  MFarSpell,
  MLoadingDictionary,
  MEngineNotInstalled,
  MNoDictionaries,

  MSuggestion,
  MReplace,
  MSkip,
  MStop,
  MPreferences,
  MEditorGeneralConfig,
  MUnloadDictionaries,

  MHighlight,
  MDictianary,
  MParser,

  MGeneralConfig,
  MPluginEnabled,
  MSpellExts,
  MSuggMenu,
  MFileSettings,
  MAnotherColoring,
  MColorSelectBtn,

  MSelectColor,
  MForeground,
  MBackground,
  MTest,

  MFileSettingWasDisabled,
  MWillDeleteFileSetting,
  MFileSettingsRemoved,

  M_FMT_AUTO_TEXT,
  M_FMT_TEXT,
  M_FMT_LATEX,
  M_FMT_HTML,
  M_FMT_MAN,
  M_FMT_FIRST,
};

// Dialog ID's
enum {
  ID_GC_SpellExts=300,
};

#ifdef _DEBUG
class C { public: int c; C():c(0x10){} virtual void c_proc() {} };
class C1: public virtual C { public: int c1; C1():c1(1){C::c=0x20;} /*virtualvoid c1_proc() {}*/  };
class C2: public virtual C { public: int c2; C2():c2(2){C::c=0x30;} virtual void c2_proc() {} };
class C1_2:
  public C1,
  public C2,
  public virtual C,
  public IUnknown
  {
        int cnt;
        HRESULT __stdcall IUnknown::QueryInterface(const IID &,void ** ) { return E_FAIL; }
        ULONG __stdcall IUnknown::AddRef(void) {  return E_FAIL;  }
        ULONG __stdcall IUnknown::Release(void) { return E_FAIL;  }
  public:
        virtual void c1_2() {}
    int c3;
    C1_2(int x):c3(3) {}
  };

bool enable_bug = false;
void Bug()
{
  if (!enable_bug) return;
  enable_bug = false;
  typedef enum { A=0, B, C,D=0xF } t_enum;
  typedef union { char* s; int i; } t_union;
  t_enum _en_var = D;
  int *w=(int*)1;
  void *x=MessageBoxA;
  wchar_t wc[11] = L"wide test!";
  register int reg = 123;
  t_union un = {"union"};
  C1_2 z(1);
  /*fprintf(stderr, "virtual address = %X %X %X %X\r\n",
    z.C1_2::C1::c,
    z.C1_2::C2::c,
    &z.C1_2::C1::c,
    &z.C1_2::C2::c);*/
  *w=1;
}
#endif _DEBUG

enum { FMT_AUTO_TEXT, FMT_TEXT, FMT_LATEX, FMT_HTML, FMT_MAN, FMT_FIRST };


template <class  _Parser>
TextParser * createParser(Hunspell* speller)
{
  if (struct unicode_info2 * uconv = speller->get_utf_conv())
  {
    int wcs_len;
    unsigned short * wcs = speller->get_wordchars_utf16(&wcs_len);
    return new _Parser(uconv, wcs, wcs_len);
  } else
    return new _Parser(speller->get_wordchars());
}

template <> TextParser * createParser<FirstParser>(Hunspell* speller)
{
  return new FirstParser(speller->get_wordchars());
}

TextParser * newParser(Hunspell* speller, int format, const char * extension)
// from hunspell-1.1.2/src/tools/hunspell.cxx
{
    TextParser * p = NULL;

    switch (format)
    {
      case FMT_TEXT: p = createParser<TextParser>(speller); break;
      case FMT_LATEX: p = createParser<LaTeXParser>(speller); break;
      case FMT_HTML: p = createParser<HTMLParser>(speller); break;
      case FMT_MAN: p = createParser<ManParser>(speller); break;
      case FMT_FIRST: p = createParser<FirstParser>(speller);
    }

    if ((!p) && (extension)) {
  if ((strcmp(extension, ".html") == 0) ||
          (strcmp(extension, ".htm") == 0) ||
          (strcmp(extension, ".xhtml") == 0) ||
          (strcmp(extension, ".docbook") == 0) ||
          (strcmp(extension, ".sgml") == 0) ||
      (strcmp(extension, ".xml") == 0)) {
          p = createParser<HTMLParser>(speller);
  } else if (((extension[0] > '0') && (extension[0] <= '9'))) {
          p = createParser<ManParser>(speller);
  } else if ((strcmp(extension, ".tex") == 0)) {
          p = createParser<LaTeXParser>(speller);
  }
    }

    if (!p)
      p = createParser<TextParser>(speller);
    return p;
}


FarString HRString(HRESULT hr)
{
  char *msg;
  FormatMessage (
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    0,
    hr,
    MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
    reinterpret_cast<char *> (&msg),
    0,
    0);
  FarString res(msg);
  LocalFree(msg);
  return res;
}

void ConvertEncoding(FarString &src, int src_enc, FarString &dst, int dst_enc)
{
  wchar_t *w;
  int wc, ac;
  if (src_enc==dst_enc)
  {
    if (&src!=&dst) dst=src;
    return;
  }
  wc = MultiByteToWideChar(src_enc, 0, src.c_str(), src.Length(), NULL, 0);
  w = (wchar_t*)alloca((wc+1)*sizeof(wchar_t));
  wc = MultiByteToWideChar(src_enc, 0, src.c_str(), src.Length(), w, wc);
  ac = WideCharToMultiByte(dst_enc, 0, w, wc, NULL, 0, NULL, NULL);
  ac = WideCharToMultiByte(dst_enc, 0, w, wc, dst.GetBuffer(ac+1), ac, NULL, NULL);
  dst.ReleaseBuffer(ac);
}

class FarEditorSuggestList;
class FarSpellEditor
{
  friend class FarEditorSuggestList;
  public:
    int EditorId;
    FarSpellEditor *next, *prev;
    class Manager
    {
      public:
        HMODULE hDll;
        FarSpellEditor *last;
        FarFileName plugin_root, dict_root;
        FarArray<FarString> cache_engine_langs;
        FarArray<Hunspell> cache_engine_instances;
        FarString highlight_list;
        bool plugin_enabled;
        bool enable_file_settings;
        int highlight_color;
        bool suggestions_in_menu;
        bool highlight_deletecolor; 
        class FarRegistry1: public FarRegistry
        {  public:
             FarRegistry1 (const char *rootKeyStart, const char *rootKeyBody)
             : FarRegistry(rootKeyStart, rootKeyBody) {}
           int DeleteRegValue (const char *RootKey, const char *ValName, HKEY hRoot = HKEY_CURRENT_USER)
           {
             HKEY hKey = OpenRegKey (hRoot, RootKey, false);
             int ExitCode = RegDeleteValue (hKey, ValName);
             RegCloseKey(hKey);
             return ExitCode;
           }
        };
        FarRegistry1 reg;
#       ifndef HARDCODED_MLDATA
        IMultiLanguage2 *ml;
#       endif HARDCODED_MLDATA
        Manager():
          reg(Far::GetRootKey(), "\\FarSpell"),
          plugin_root(FarFileName(Far::GetModuleName()).GetPath()),
          dict_root(FarFileName(plugin_root+"dict\\"))
        {
          hDll = GetModuleHandle(Far::GetModuleName());
          last = NULL;
          plugin_enabled = reg.GetRegKey("", plugin_enabled_key, true);
          enable_file_settings = reg.GetRegKey("", enable_file_settings_key, true);
          highlight_list = reg.GetRegStr("", highlight_mask_key, "*.txt,*.,*.tex,*.htm,*.html,*.docbook");
          highlight_color = reg.GetRegKey("", highlight_color_key, 0x84);
          suggestions_in_menu = reg.GetRegKey("", suggestions_in_menu_key, 0x1);
          highlight_deletecolor = reg.GetRegKey("", highlight_deletecolor_key, false);
#         ifndef HARDCODED_MLDATA
          HRESULT hr;
          ml = NULL;
          hr = CoInitialize(NULL);
          if (FAILED(hr))
            GetLog().Message("CoInitialize=0x%x %s", hr, HRString(hr).c_str());
          else
          {
            hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_SERVER,
                                IID_IMultiLanguage2, (void**)&ml);
            if (FAILED(hr))
              GetLog().Message("CoCreateInstance(IMultiLanguage2)=0x%x %s", hr, HRString(hr).c_str());
          }
#         endif  HARDCODED_MLDATA
        }
        ~Manager()
        {
          reg.SetRegKey("", plugin_enabled_key, plugin_enabled);
          reg.SetRegKey("", enable_file_settings_key, enable_file_settings);
          reg.SetRegKey("", highlight_mask_key, highlight_list);
          reg.SetRegKey("", highlight_color_key, highlight_color);
          reg.SetRegKey("", suggestions_in_menu_key, suggestions_in_menu);
          reg.SetRegKey("", highlight_deletecolor_key, highlight_deletecolor);
          while (last) delete last;
#         ifndef HARDCODED_MLDATA
          if (ml)
          {
            ml->Release();
            ml = NULL;
          }
          CoUninitialize();
#         endif  HARDCODED_MLDATA
          editors = NULL;
        }
        void UnloadDictionaries()
        {
          cache_engine_langs.Clear();
          cache_engine_instances.Clear();
        }
        FarSpellEditor* Lookup(int id)
        {
          FarSpellEditor *editor;
          editor = last;
          while (editor)
          {
            if (id == editor->EditorId)
              return editor;
            editor = editor->prev;
          }
          return NULL;
        }
        Hunspell* GetDictInstance(FarString& dict);
        FarLog GetLog()
        {
          return FarLog(plugin_root+"farspell.log");
        }
        int OnConfigure(int ItemNumber);
          int GeneralConfig(bool from_editor);
          int ColorSelectDialog();
          void ClearFileSettings();
        int GetCharsetEncoding(FarString &name);
        int OnEvent(int Event, int Param);
        void OnMenu()
        {
           FarEdInfo fei;
           FarSpellEditor *editor = Lookup(fei.EditorID);
           if (editor)
              editor->DoMenu(fei, suggestions_in_menu);
        }
    };
    static Manager *editors;
    static void Init()
    {
      editors = new Manager();
    }
  private:
    char *default_config_string;
    int highlight;
    FarFileName file_name;
    Hunspell* _dict_instance;
    FarString dict;
    TextParser* _parser_instance;
    int parser_id;
    int spell_enc, doc_enc, doc_tablenum, doc_ansi;
    int colored_begin, colored_end;
    enum { RS_DICT = 0x1, RS_PARSER = 0x2,
           RS_ALL = RS_DICT|RS_PARSER };
    void RecreateEngine(int what = RS_ALL);
    void DropEngine(int what = RS_ALL);
    Hunspell* GetDict();
    TextParser* GetParser();
  public:
    FarSpellEditor();
    ~FarSpellEditor();
    void DoMenu(FarEdInfo &fei, bool insert_suggestions);
    void Save(FarEdInfo *fei);
    void Redraw(FarEdInfo &fei, int What);
    void HighlightRange(FarEdInfo &fei, int top_line, int bottom_line);
    void ClearAndRedraw(FarEdInfo &fei);
    void ShowPreferences(FarEdInfo &fei);
    int ShowSuggestion(int line, int start, int len, const char* word, char ** wlst, int ns);
    void UpdateDocumentCharset(FarEdInfo &fei);
    virtual void ReportError (int line, int col, const char *expected)
    {
       editors->GetLog().Error("%s:%d:%d: expected %s", file_name.c_str(), line, col, expected);
    }
};

FarSpellEditor::Manager *FarSpellEditor::editors = NULL;

Hunspell* FarSpellEditor::Manager::GetDictInstance(FarString& dict)
{
  for (int i = 0; i < cache_engine_langs.Count(); i++)
  {
     if ((*cache_engine_langs[i]) == dict)
       return cache_engine_instances[i];
  }
  HANDLE screen = Far::SaveScreen();
  FarMessage wait(0);
  wait.AddLine(MFarSpell);
  wait.AddFmt(MLoadingDictionary, dict.c_str());
  wait.Show();
  Hunspell* engine = new Hunspell(
         FarFileName::MakeName(dict_root, dict+".aff").c_str(),
         FarFileName::MakeName(dict_root, dict+".dic").c_str());
  cache_engine_langs.Add(new FarString(dict));
  cache_engine_instances.Add(engine);
  Far::RestoreScreen(screen);
  return engine;
}

int FarSpellEditor::Manager::GetCharsetEncoding(FarString &name)
{
# ifdef HARDCODED_MLDATA
  int res = search_codepage(name);
  if (res != -1) return res;
  GetLog().Error("GetCharsetInfo: %s - not found", name.c_str());
  // TODO: notify user
# else
  wchar_t *caption; int caption_len;
  BSTR bcaption;
  MIMECSETINFO mcsi;
  HRESULT hr;
  //
  caption_len = mbstowcs(NULL, name.c_str(), name.Length());
  caption = (wchar_t *)alloca((caption_len+1)*sizeof(wchar_t));
  mbstowcs(caption, name.c_str(), name.Length());
  bcaption = SysAllocStringLen(caption, caption_len);
  hr = ml->GetCharsetInfo(bcaption, &mcsi);
  SysFreeString(bcaption);
  if (SUCCEEDED(hr))
    return mcsi.uiInternetEncoding;
  GetLog().Error("GetCharsetInfo %s: %x %s", name.c_str(), hr, HRString(hr).c_str());
  // TODO: notify user
# endif HARDCODED_MLDATA
  return GetACP();
}

void FarSpellEditor::Manager::ClearFileSettings()
{
  reg.DeleteRegKey("", editor_files_key);
}

// config format: int highlight | string dictinoary | int parser_id
char *const config_template = "%d|%s|%d";
char *const default_config_hl = "1|en_US|0";
char *const default_config_nhl = "0|en_US|0";

FarSpellEditor::FarSpellEditor():
  dict("en_US")
{
  //============================
  next = NULL;
  if (prev = editors->last)
    editors->last->next = this;
  editors->last = this;
  //============================
  FarEdInfo fei;
  EditorId = fei.EditorID,
  //editors->GetLog().Message("Creatig 0x%X (%d)", this, EditorId);
  file_name = fei.FileName;
  highlight = 1;
  default_config_string = default_config_hl;
  parser_id = FMT_AUTO_TEXT;
  _dict_instance = NULL;
  _parser_instance = NULL;
  colored_begin = INT_MAX;
  colored_end = 0;

  FarString config = editors->reg.GetRegStr(editor_files_key, file_name, "");
  if (config.Length())
  {
    //editors->GetLog().Message("%s=%s", file_name.c_str(), config.c_str());
    FarStringTokenizer tokenizer (config, '|');
    int i = 0;
    while (tokenizer.HasNext())
    {
      switch (i)
      {
        case 0: highlight = atoi(tokenizer.NextToken()); break;
        case 1: dict = tokenizer.NextToken(); break;
        case 2: parser_id = atoi(tokenizer.NextToken()); break;
      }
      i++;
    }
  }
  else
  {
    highlight = FarSF::CmpNameList(editors->highlight_list, file_name, true);
    default_config_string = highlight ? default_config_hl : default_config_nhl;
  }
  //RecreateEngine(RS_ALL);
}

FarSpellEditor::~FarSpellEditor()
{
  Save(NULL);
  DropEngine(RS_ALL);
  //===============================
  if (this == editors->last)
  {
    editors->last = prev;
    if (next) editors->GetLog().Error("Assertion failed: 20051220 EditorId=%d next.EditorId=%d",
      EditorId, next->EditorId);
  }
  if (next)
  {
    next->prev = prev;
    next = NULL;
  }
  if (prev)
  {
    prev->next = next;
    prev = NULL;
  }
  //===============================
}

void FarSpellEditor::Save(FarEdInfo *fei)
{
  if (!editors->enable_file_settings) return;
  if (!FarFileInfo::IsDirectory(file_name.GetPath())) throw "invalid file";
  int size = sizeof(config_template)+dict.Length()+32*2;
  char *config_text = (char*)alloca(size);
  _snprintf(config_text, size-sizeof(char), config_template,
     highlight,
     dict.c_str(),
     parser_id);
  if (strcmp(default_config_string, config_text))
    editors->reg.SetRegKey(editor_files_key, file_name, config_text);
  else
    editors->reg.DeleteRegValue(editor_files_key, file_name); // reconstruction possible
}

void FarSpellEditor::DropEngine(int what)
{
  _dict_instance = NULL;
  if (_parser_instance)
  {
    delete _parser_instance;
    _parser_instance = NULL;
  }
}

void FarSpellEditor::RecreateEngine(int what)
{
  if (what&(RS_DICT))
  {
    _dict_instance = editors->GetDictInstance(dict);
    spell_enc = editors->GetCharsetEncoding(FarString(_dict_instance->get_dic_encoding()));
  }
  if (what&(RS_PARSER) && GetDict())
  {
    if (_parser_instance) delete _parser_instance;
    _parser_instance = newParser(GetDict(), parser_id, file_name.GetExt());
  }
}

Hunspell* FarSpellEditor::GetDict()
{
  if (!_dict_instance) RecreateEngine(RS_DICT);
  return _dict_instance;
}

TextParser* FarSpellEditor::GetParser()
{
  if (!_parser_instance) RecreateEngine(RS_PARSER);
  return _parser_instance;
}

int FarSpellEditor::Manager::OnEvent(int Event, int Param)
{
  FarSpellEditor *editor;
  int i, id=-1;
  switch (Event)
   {
    case EE_READ: // Param==NULL
      new FarSpellEditor();
      break;
    case EE_SAVE: // Param==NULL
      if (plugin_enabled) {
        FarEdInfo fei;
        editor=Lookup(id=fei.EditorID);
        if (editor) editor->Save(&fei);
        else goto error;
      }
      break;
    case EE_CLOSE: // *Param==EditorId || Param==NULL;
      if (Param)
      {
        editor=Lookup(id=*(int*)Param);
        //GetLog().Message("OnEvent(%d,%d): Closing 0x%X(%d)", Event, Param, editor, id);
        if (editor) delete editor;
        else goto error;
      }
      break;
    case EE_REDRAW:
      if (plugin_enabled) {
        FarEdInfo fei;
        editor=Lookup(id=fei.EditorID);
        if (editor) editor->Redraw(fei, Param);
        else goto error;
      }
      break;
  }
  return 0;
error:
  GetLog().Error("OnEvent(%d,%d): FarSpellEditor[%d] instance not found", Event, Param, id);
  return 0;
}

void FarSpellEditor::Redraw(FarEdInfo &fei, int What)
{
  if (!highlight || !GetDict() || !GetParser()) return;
  switch (What)
  {
     case EEREDRAW_ALL:
     case EEREDRAW_CHANGE:
       HighlightRange(fei, fei.TopScreenLine, fei.TopScreenLine+fei.WindowSizeX-1);
       break;
     case EEREDRAW_LINE:
       HighlightRange(fei, fei.CurLine, fei.CurLine);
       break;
  }
#ifdef _DEBUG
  Bug();
#endif _DEBUG
}


void FarSpellEditor::UpdateDocumentCharset(FarEdInfo &fei)
{
  if (doc_tablenum == fei.TableNum && doc_ansi == fei.AnsiMode) return;
  if (fei.TableNum != -1)
  {
    FarString doc_cs;
    CharTableSet table;
    Far::GetCharTable(fei.TableNum, &table);
    FarString keyName, far_tn(table.TableName);
    FarRegistry table_registry ("Software\\Far\\CodeTables");
    FarRegistry::KeyIterator keyIter = table_registry.EnumKeys("", HKEY_CURRENT_USER);
    while (keyIter.NextKey (keyName))
    {
      FarString tn(table_registry.GetRegStr (keyName, "TableName", "", HKEY_CURRENT_USER));
      //editors->GetLog().Message("key=%s %s", keyName.c_str(), tn.c_str());
      if (tn==far_tn)
      {
        //editors->GetLog().Message("hit!");
        doc_cs = table_registry.GetRegStr (keyName, "RFCCharset", "", HKEY_CURRENT_USER);
        break;
      }
    }
    doc_enc = editors->GetCharsetEncoding(doc_cs);
  }
  else
    doc_enc = fei.AnsiMode ? GetACP() : GetOEMCP();
  doc_tablenum = fei.TableNum;
  doc_ansi = fei.AnsiMode;
  //editors->GetLog().Message("tn=%d doc_cs=%s spell_cs=%s spell_enc=%d doc_enc=%d", fei.TableNum,  doc_cs.c_str(), spell_cs.c_str(), spell_enc, doc_enc);
}

void FarSpellEditor::HighlightRange(FarEdInfo &fei, int top_line, int bottom_line)
{
  FarString document;
  char *token;
  Hunspell *dict_inst = GetDict();
  TextParser *parser_inst = GetParser();

  UpdateDocumentCharset(fei);
  colored_begin = min(colored_begin, top_line);
  colored_end = max(colored_end, bottom_line);
  //GetLog().Message("top=%d bot=%d", top_line, bottom_line);
  for (int line_no=top_line; line_no<bottom_line; line_no++)
  {
    if (editors->highlight_deletecolor)
      FarEd::DeleteColor(line_no);
    document = FarEd::GetStringText(line_no);
    document.Delete(MAXLNLEN, document.Length());
    ConvertEncoding(document, doc_enc, document, spell_enc);
    parser_inst->put_line((char*)document.c_str());
    while ((token = parser_inst->next_token()))
    {
      if (!dict_inst->spell(token))
      {
        int pos = parser_inst->get_tokenpos();
        FarEd::AddColor(line_no, pos, pos+strlen(token)-1, editors->highlight_color);
      }
      hunspell_free(token);
    }
  }
}

void FarSpellEditor::ClearAndRedraw(FarEdInfo &fei)
{
  if (!highlight) return;
  if (editors->highlight_deletecolor && colored_begin!=INT_MAX && colored_end!=0)
    for (int i=colored_begin; i<min(colored_end, fei.TotalLines); i++)
      FarEd::DeleteColor(i);
  colored_begin=INT_MAX;
  colored_end=0;
  Redraw(fei, (int)EEREDRAW_ALL);
}

int FarSpellEditor::ShowSuggestion(int line, int start, int len, const char* word, char ** wlst, int ns)
{
  FarEdInfo fei;
  FarDialog dialog(MSuggestion);
  int scr_x;
  int scr_y;
  FarString str_word(word, len);
  ConvertEncoding(str_word, spell_enc, str_word, GetOEMCP());
  FarComboBox dlg_variants(&dialog, &str_word, -1, 32);
  FarButtonCtrl dlg_replace(&dialog, MReplace);
  FarButtonCtrl dlg_preferences(&dialog, MPreferences);
  FarButtonCtrl dlg_cancel(&dialog, MCancel);
  FarControl *res;
  FarString s;

  for (int j = 0; j < ns; j++)
  {
    s = wlst[j];
    ConvertEncoding(s, spell_enc, s, GetOEMCP());
    dlg_variants.AddItem(s);
  }
  res = dialog.Show();
  if (res == &dlg_replace)
  {
    FarEd::SetPos(line, start);
    for (int i=0; i<len; i++)
      FarEd::DeleteChar();
    s = wlst[dlg_variants.GetListPos()];
    ConvertEncoding(s, spell_enc, s, GetOEMCP());
    FarEd::InsertText(s);
    return 1;
  }
  if (res == &dlg_preferences) return 0;
  return -1;
}


int WINAPI ScanDicts(
  const WIN32_FIND_DATA *FData,
  const char *FullName,
  void *Param
)
{
  FarComboBox *dlg_languages = (FarComboBox*)Param;
  FarFileName name(FullName, strlen(FullName)-4); // *.aff only
  if (FarFileInfo::FileExists(name+".dic"))
    dlg_languages->AddItem(name.GetNameExt());
  return 1;
}

void FarSpellEditor::ShowPreferences(FarEdInfo &fei)
{
  static int parsers[] =
  {
    M_FMT_AUTO_TEXT,
    M_FMT_TEXT,
    M_FMT_LATEX,
    M_FMT_HTML,
    M_FMT_MAN,
    M_FMT_FIRST,
  };
  FarDialog dialog(MPreferences, "Contents");
  FarCheckCtrl dlg_highlight(&dialog, MHighlight, highlight);
  FarTextCtrl dlg_languages_label(&dialog, MDictianary, -1);
  FarComboBox dlg_languages(&dialog, &dict, -1, 32);
  FarString s(Far::GetMsg(parsers[parser_id]));
  FarTextCtrl dlg_parser_label(&dialog, MParser, -1);
  FarComboBox dlg_parser(&dialog, &s, -1, 32);
  FarButtonCtrl dlg_ok(&dialog, MOk, DIF_CENTERGROUP);
  FarButtonCtrl dlg_cancel(&dialog, MCancel, DIF_CENTERGROUP);
  FarControl *res;

  FarSF::RecursiveSearch((char*)editors->dict_root.c_str(), "*.aff", ScanDicts, 0, &dlg_languages);
  for (int i=0; i<sizeof(parsers)/sizeof(parsers[0]); i++)
    dlg_parser.AddItem(Far::GetMsg(parsers[i]));
  dlg_languages.SetFlags(DIF_DROPDOWNLIST);
  dlg_parser.SetFlags(DIF_DROPDOWNLIST);
  dialog.SetDefaultControl(&dlg_ok);

  res=dialog.Show();

  if (res == &dlg_ok)
  {
    FarString new_dict = dlg_languages.GetText();
    int new_parser_id = dlg_parser.GetListPos();
    highlight = dlg_highlight.GetSelected();
    if (new_dict.Length() && dict != new_dict)
    {
      dict = new_dict;
      if (new_parser_id != -1) parser_id = new_parser_id;
      DropEngine(RS_ALL);
      ClearAndRedraw(fei);
    }
    else if (new_parser_id != -1  &&  new_parser_id != parser_id)
    {
      parser_id = new_parser_id;
      DropEngine(RS_PARSER);
      ClearAndRedraw(fei);
    }
  }
}

class FarEditorSuggestList
{
    char **wlst;
    int tpos, tlen;
    int line;
    char *token;
    int ns;
    FarSpellEditor* editor;
    FarString _word_cache;
  public:
    FarEditorSuggestList(FarEdInfo &fei, FarSpellEditor* _editor)
    : editor(_editor)
    {
      token = NULL;
      wlst = NULL;
      ns = 0;
      Hunspell *dict_inst = editor->GetDict();
      TextParser *parser_inst = editor->GetParser();
      int cpos = fei.CurPos;
      line = fei.CurLine;
      FarString document(FarEd::GetStringText(line));

      ConvertEncoding(document, editor->doc_enc, document, editor->spell_enc);
      parser_inst->put_line((char*)document.c_str());
      while ((token = parser_inst->next_token()))
      {
        if (!dict_inst->spell(token))
        {
          tpos = parser_inst->get_tokenpos();
          tlen = strlen(token);
          if (cpos>=tpos && cpos<(tpos+tlen))
            break;
        }
        hunspell_free(token);
        token = NULL;
      }
      if (token)
      {
        ns = dict_inst->suggest(&wlst, token);
        hunspell_free(token);
        token = NULL;
      }
    }
    ~FarEditorSuggestList()
    {
      if (wlst)
      {
        for (int j = 0; j < ns; j++)
          hunspell_free(wlst[j]);
        hunspell_free(wlst);
        wlst = NULL;
      }
      if (token)
      {
        hunspell_free(token);
        token = NULL;
      }
    }
    inline int Count() const
    {
      return ns;
    }
    FarString& operator[](int index) 
    {
      _word_cache = wlst[index];
      ConvertEncoding(_word_cache, editor->spell_enc, _word_cache, GetOEMCP());
      return _word_cache;
    }
    bool Apply(int index) 
    {
      if (!ns || !wlst || index>=ns || index < 0) throw "Bad Apply";
      FarEd::SetPos(line, tpos);
      for (int i=0; i<tlen; i++)
        FarEd::DeleteChar();
      FarString s(wlst[index]);
      ConvertEncoding(s, editor->spell_enc, s, GetOEMCP());
      FarEd::InsertText(s);
    }
};

void FarSpellEditor::DoMenu(FarEdInfo &fei, bool insert_suggestions)
{
  FarEditorSuggestList *sl 
   = insert_suggestions && editors->plugin_enabled 
     ? new FarEditorSuggestList(fei, this) : NULL;
  FarMenuEx menu(MFarSpell, FMENU_WRAPMODE, "Contents");
  int i, static_part = 0;

  if (sl && sl->Count())
  {
    for (int j = 0; j < sl->Count(); j++) 
      menu.AddItem((*sl)[j]);
    menu.AddSeparator();
    static_part = sl->Count()+1;
  }
  i = menu.AddItem(MPreferences);
  if (!editors->plugin_enabled)
    menu.DisableItem(i);

  menu.AddItem(MEditorGeneralConfig);

# ifdef _DEBUG
  i = menu.AddItem("Test dump");
  if (!editors->plugin_enabled)
    menu.DisableItem(i);
# endif _DEBUG

  int res = menu.Show();

  if (res==-1)
  {
    //pass
  }
  else if (sl && sl->Count() && res<sl->Count())
  {
    sl->Apply(res);
  }
  else
  {
    switch (res-static_part)
    {
      case 0: ShowPreferences(fei); break;
      case 1: editors->GeneralConfig(true); break;
#     ifdef _DEBUG
      case 2: enable_bug = true;
#     endif _DEBUG
    }
  }
  if (sl)
  {
    delete sl;
    sl = NULL;
  }
}

int FarSpellEditor::Manager::ColorSelectDialog()
{
  DWORD dlg; // Dialog handle
  int res;
  int col1 = highlight_color&0x0F;
  int col2 = highlight_color&0xF0;
  dlg = LoadDialog(hDll, "farspell.dlg", "Color Select"); // Load dialog
  SetRadioStatus(dlg, 1, col1);
  SetRadioStatus(dlg, 16, col2);
  do // thanks to Semenov Alexey for very useful example ;)
  {
    SetItemColor(dlg, 243, col1+col2);  // ���������� ���� ��� "�������"
    // ���������� ��������� �������, Id �������� - ��� �������� �
    // ����������� Id ������� ����.

    res = ShowDialog(dlg);  // �������������� ������.
    col1 = GetRadioStatus(dlg, 1);   // �������� ��������� �����.
    col2 = GetRadioStatus(dlg, 16);  // ���������� ��������� � Id
  }
  // ��������� ���� ������ �� ������
  // ��� ����������.
  // 242 - Id ������ "��"
  // -1  - Id ������ "��������" �
  // ��� ������������ ��� ESC.
  while( res!=242 && res!=-1 );

  if (res!=-1)
  {
    // ���� ������ �� ��� ���������
    // �� ��������� ����� �����.
    highlight_color = col1+col2;
  }
  FreeDialog(dlg); // Free dialog
  return FALSE;
}

int FarSpellEditor::Manager::GeneralConfig(bool from_editor)
{
  DWORD dlg; // Dialog handle
  int res;
  bool last_enable_file_settings = enable_file_settings;
  dlg = LoadDialog(hDll, "farspell.dlg", "General config"); // Load dialog
  char* p = GetItemData(dlg, ID_GC_SpellExts);
  strcpy(p, highlight_list.c_str());
  SetItemStatus(dlg, MPluginEnabled, plugin_enabled);
  SetItemStatus(dlg, MSuggMenu, suggestions_in_menu);
  SetItemStatus(dlg, MAnotherColoring, !highlight_deletecolor);
  SetItemStatus(dlg, MFileSettings, enable_file_settings);
  if (from_editor)
    HideItem(dlg, MUnloadDictionaries);

  for (int cont=1; cont;) switch (ShowDialog(dlg))
  {
    case MColorSelectBtn:
      ColorSelectDialog();
      break;
    case MUnloadDictionaries:
      UnloadDictionaries();
      break;
    case MOk:
      {
        int l = strlen(p);
        char* news = highlight_list.GetBuffer(l);
        strncpy(news, p, l);
        highlight_list.ReleaseBuffer(l);
        suggestions_in_menu = GetItemStatus(dlg, MSuggMenu); 
        highlight_deletecolor = !GetItemStatus(dlg, MAnotherColoring);
        plugin_enabled = GetItemStatus(dlg, MPluginEnabled);
        enable_file_settings = GetItemStatus(dlg, MFileSettings);
      }
      cont = 0;
      break;
    case -1: // cancel
      cont = 0;
      break;
  }
  FreeDialog(dlg); // Free dialog
  if (!enable_file_settings && last_enable_file_settings)
  {
    FarMessage msg(FMSG_MB_YESNO);
    msg.AddLine(MGeneralConfig);
    msg.AddLine(MFileSettingWasDisabled);
    msg.AddLine(MWillDeleteFileSetting);
    if (msg.Show()==0)
    {
      ClearFileSettings();
      FarMessage().SimpleMsg(FMSG_MB_OK, MGeneralConfig, MFileSettingsRemoved, -1);
    }
  }
  return FALSE;
}

int FarSpellEditor::Manager::OnConfigure(int ItemNumber)
{
  return GeneralConfig(false);
}

// -- Exported plugin functions ----------------------------------------------
#ifdef _DEBUG
class ExcDump
{
  public:
    int code;
    static bool no_more;
    ExcDump(PEXCEPTION_POINTERS pExInfo)
    {
      try
      {
        {
          if (!no_more)
          {
            FarMessage wait( FMSG_WARNING );
            wait.AddFmt( MExceptionIn, FarFileName(Far::GetModuleName()).GetNameExt().c_str());
            wait.AddLine( MDbgPleaseWait );
            wait.Show();
          }
          FarDumpStackCb(pExInfo, sDump, this);
          ToLog();
        }
        ToMessage();
      }
      catch(...)
      {
      }
      code = EXCEPTION_EXECUTE_HANDLER;
    }
    void ToLog()
    {
      FarLog log(FarString(Far::GetModuleName())+".err!");
      log.Error("exception");
      log.Write(dump.c_str(), dump.Length());
    }
    void ToMessage()
    {
      if (no_more) return;
      FarMessage Msg( FMSG_WARNING|FMSG_LEFTALIGN, "Exception" );
      FarStringTokenizer lines(dump, '\r');
      int limit = 15, skip_lf=0;
      Msg.AddFmt( MExceptionIn, FarFileName(Far::GetModuleName()).GetNameExt().c_str());
      // lines.NextToken(); // skip
      while (limit-- && lines.HasNext())
      {
        Msg.AddLine( &lines.NextToken().c_str()[skip_lf] );
        skip_lf=1;
      }
      Msg.AddFmt( MDbgSeeMore, Far::GetModuleName());
      Msg.AddSeparator();
      Msg.AddButton( MDbgContinue );
      Msg.AddButton( MNoDbgMessages );
      Msg.AddButton( MUnloadPlugin );
      Msg.AddButton( MExitFAR );

      switch (Msg.Show())
      {
        case -1:
        case 0:  // Continue
          break;
        case 1:  // Don''t disturb me
          no_more = 1;
          break;
        case 2: // Unload plugin
          //FarCtrl().ClosePlugin(NULL);
          delete FarSpellEditor::editors;
          Far::Done();
          break;
        case 3: // Exit FAR
          ExitThread(0);
          break;
      }
    }
  private:
    FarString dump;
    void Dump(void* param, const char* Fmt, va_list Args)
    {
      char tmp[1024];
      wvsprintf( tmp, Fmt, Args );
      dump+=tmp;
      /*if (str.Length() && str[str.Length()-1] == '\n')
      {
        log.Error("%s", str.c_str());
        str.Empty();
      }*/
    }
    static void sDump(void* const param, const char* Fmt, ...)
    {
      ExcDump *const pThis = (ExcDump*)param;
      va_list argPtr;
      va_start( argPtr, Fmt );
      pThis->Dump(param, Fmt, argPtr);
      va_end( argPtr );
    }
};
bool ExcDump::no_more = false;

int far_exc_dump(PEXCEPTION_POINTERS pExInfo)
{
  try
  {
    return ExcDump(pExInfo).code;
  }
  catch(...)
  {
    return EXCEPTION_EXECUTE_HANDLER;
  }
}
#endif _DEBUG

int WINAPI _export SetLngMode() //Exported for DialogGenerator
{
  return 0;
}

void WINAPI _export SetStartupInfo (const struct PluginStartupInfo *Info)
{
#ifdef _DEBUG
  __try
  {
#endif _DEBUG
    Far::Init (Info, PF_EDITOR|PF_DISABLEPANELS);
    Far::AddPluginMenu (MFarSpell);
    Far::AddPluginConfig (MFarSpell);
    FarSpellEditor::Init();
    DialogInit(FarSpellEditor::editors->hDll, *Info);
#ifdef _DEBUG
  }
  __except(far_exc_dump(GetExceptionInformation()))
  {
  }
#endif _DEBUG
}

HANDLE WINAPI _export OpenPlugin (int /*OpenFrom*/, int /*Item*/)
{
  if (FarSpellEditor::editors) 
#ifdef _DEBUG
  __try
  {
#endif _DEBUG
    FarSpellEditor::editors->OnMenu();
#ifdef _DEBUG
  }
  __except(far_exc_dump(GetExceptionInformation()))
  {
  }
#endif _DEBUG
  return INVALID_HANDLE_VALUE;
}

void WINAPI _export ExitFAR()
{
  if (FarSpellEditor::editors)
  {
    delete FarSpellEditor::editors;
    Far::Done();
  }
}

void WINAPI _export GetPluginInfo (struct PluginInfo *Info)
{
  Far::GetPluginInfo (Info);
}

int WINAPI _export Configure(int ItemNumber)
{
#ifdef _DEBUG
  __try
  {
#endif _DEBUG
    return FarSpellEditor::editors->OnConfigure(ItemNumber);
#ifdef _DEBUG
  }
  __except(far_exc_dump(GetExceptionInformation()))
  {
  }
  return 0;
#endif _DEBUG
}


int WINAPI _export ProcessEditorEvent(int Event, int Param)
{
#ifdef _DEBUG
  __try
  {
#endif _DEBUG
    return FarSpellEditor::editors?FarSpellEditor::editors->OnEvent(Event, Param):0;
#ifdef _DEBUG
  }
  __except(far_exc_dump(GetExceptionInformation()))
  {
  }
  return 0;
#endif _DEBUG
}