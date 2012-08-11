//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "../RtCA.h"
//------------------------------------------------------------------------------
//init strings + tests name + menu
//------------------------------------------------------------------------------
void InitGlobalLangueString(unsigned int langue_id)
{
  current_lang_id = langue_id;

  //get global GUI and message strings
  FORMAT_CALBAK_READ_INFO fcri;
  fcri.type = TYPE_SQLITE_FLAG_LANG_INIT_STRINGS;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

  if (CONSOL_ONLY == FALSE)
  {
    //update menu
    FORMAT_CALBAK_READ_INFO fcri;
    fcri.type = TYPE_SQLITE_FLAG_LANGUAGE_CHANGE;
    SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

    if (current_item_selected > -1)
    {
      fcri.type = TYPE_SQLITE_FLAG_LANGUAGE_COL_CHANGE;
      SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);
    }

    //update list of tests
    SendMessage(hlstbox, LB_RESETCONTENT,0,0);
    fcri.type = TYPE_SQLITE_FLAG_TESTS_INIT;
    SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

    //update Windows
    ShowWindow(h_main, SW_HIDE);
    ShowWindow(h_main, SW_SHOW);

    //select item ! + force use of gui message
    if (current_item_selected > -1)
    {
      SendMessage(hlstbox, LB_SETCURSEL, current_item_selected, 0);
      TRI_RESULT_VIEW       = FALSE;
      column_tri            = -1;
      pos_search            = 0;

      FORMAT_CALBAK_READ_INFO fcri;

      //get column count
      fcri.type = TYPE_SQLITE_FLAG_GET_COLUM_COUNT;
      SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

      //get column + text
      fcri.type = TYPE_SQLITE_FLAG_VIEW_CHANGE;
      SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

      //get items infos + items
      ListView_DeleteAllItems(hlstv);
      fcri.type = TYPE_SQLITE_FLAG_GET_ITEMS_INFO;
      SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

      char tmp_infos[DEFAULT_TMP_SIZE];
      snprintf(tmp_infos,DEFAULT_TMP_SIZE,"Item(s) : %d",ListView_GetItemCount(hlstv));
      SendMessage(hstatus_bar,SB_SETTEXT,0, (LPARAM)tmp_infos);
    }

  }
}
//------------------------------------------------------------------------------
//init string for Internet
//------------------------------------------------------------------------------
void InitSQLStrings()
{
  //db for ANDROID, FIREFOX,CHROME
  nb_sql_ANDROID  = 0;
  nb_sql_CHROME   = 0;
  nb_sql_FIREFOX  = 0;

  FORMAT_CALBAK_READ_INFO fcri;
  fcri.type = TYPE_SQLITE_FLAG_ANDROID_INIT_STRINGS;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

  fcri.type = TYPE_SQLITE_FLAG_CHROME_INIT_STRINGS;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);

  fcri.type = TYPE_SQLITE_FLAG_FIREFOX_INIT_STRINGS;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);
}
//------------------------------------------------------------------------------
//hide DOS form
//------------------------------------------------------------------------------
void hideDOSForm()
{
char pszOldWindowTitle[MAX_PATH];
GetConsoleTitle(pszOldWindowTitle, MAX_PATH);
ShowWindow(FindWindow(NULL, pszOldWindowTitle), SW_HIDE);
}
//------------------------------------------------------------------------------
//init start configuration
//------------------------------------------------------------------------------
void InitGlobalConfig(unsigned int params, BOOL debug, BOOL acl, BOOL ads, BOOL sha, BOOL recovery, BOOL local_scan)
{
  //init globals var
  current_lang_id       = 1;
  current_session_id    = 0;
  current_item_selected = -1;

  STAY_ON_TOP           = FALSE;

  DEBUG_MODE            = debug;
  FILE_ACL              = acl;
  FILE_ADS              = ads;
  FILE_SHA              = sha;
  LOCAL_SCAN            = local_scan;

  start_scan            = FALSE;
  export_type           = 0;
  _SYSKEY[0]            = 0;  //global syskey for decrypt hash of users

  //load data for db
  InitSQLStrings();
  InitGlobalLangueString(current_lang_id);
  current_session_id = session[0];

  SetDebugPrivilege(TRUE);

  //init if 64b
  OldValue_W64b = FALSE;
  ReviewWOW64Redirect(OldValue_W64b);
}
//------------------------------------------------------------------------------
//init GUI configuration
//------------------------------------------------------------------------------
DWORD WINAPI InitGUIConfig(LPVOID lParam)
{
  //hidden DOS form
  #ifndef DEV_DEBUG_MODE
    hideDOSForm();
  #endif

  //for cross compilation bug in 64bit
  #ifndef GWL_WNDPROC
    #define GWL_WNDPROC (-4)
  #endif

  //global init
  B_AUTOSEARCH      = FALSE;
  h_AUTOSEARCH      = NULL;
  ExportStart       = FALSE;
  DEBUG_CMD_MODE    = FALSE;
  TRI_RESULT_VIEW   = FALSE;
  column_tri        = -1;
  //NB_TESTS          = 0;
  pos_search        = 0;
  current_OS[0]     = 0;
  current_OS_BE_64b = FALSE;
  nb_current_columns= 0;
  current_lang_id   = 1;

  //open sqlite file
  if (sqlite3_open(DEFAULT_SQLITE_FILE, &db_scan) != SQLITE_OK)
  {
    //if tmp sqlite file exist free !!
    if (GetFileAttributes(DEFAULT_TM_SQLITE_FILE) != INVALID_FILE_ATTRIBUTES)
    {
      DeleteFile(DEFAULT_TM_SQLITE_FILE);
    }

    sqlite3_open(DEFAULT_SQLITE_FILE, &db_scan);
  }

  //Init language cb
  HANDLE H_ImagList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 , /*nb icones*/2, 0);
  ImageList_AddIcon(H_ImagList,(HICON)LoadIcon((HINSTANCE) hinst, MAKEINTRESOURCE(ICON_LANGUAGE_EN)));
  ImageList_AddIcon(H_ImagList,(HICON)LoadIcon((HINSTANCE) hinst, MAKEINTRESOURCE(ICON_LANGUAGE_FR)));
  ImageList_SetBkColor(H_ImagList, GetSysColor(COLOR_WINDOW));

  //init la combo box des langues
  SendMessage(hCombo_lang,CBEM_SETIMAGELIST,0,(LPARAM)H_ImagList);
  FORMAT_CALBAK_READ_INFO fcri;
  fcri.type = TYPE_SQLITE_FLAG_LANG_INIT;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);
  SendMessage(hCombo_lang, CB_SETCURSEL,0,0);//default select English

  //add extended style to listview
  SendMessage(hlstv,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP|LVS_EX_GRIDLINES);

  //list of sessions !!!
  nb_session = 0;
  fcri.type  = TYPE_SQLITE_FLAG_SESSIONS_INIT;
  SQLITE_LireData(&fcri, DEFAULT_SQLITE_FILE);
  SendMessage(hCombo_session, CB_SETCURSEL,0,0);

  //icons for tests
  H_ImagList_icon = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 , /*nb icon*/2, 0);
  ImageList_AddIcon(H_ImagList_icon,(HICON)LoadIcon((HINSTANCE) GetModuleHandle(0), MAKEINTRESOURCE(ICON_FOLDER)));
  ImageList_AddIcon(H_ImagList_icon,(HICON)LoadIcon((HINSTANCE) GetModuleHandle(0), MAKEINTRESOURCE(ICON_FILE)));
  ImageList_SetBkColor(H_ImagList_icon, GetSysColor(COLOR_WINDOW));

  //all others datas
  InitGlobalConfig(0, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE);
  return 0;
}
//------------------------------------------------------------------------------
//End of GUI
//------------------------------------------------------------------------------
void EndGUIConfig(HANDLE hwnd)
{
  sqlite3_close(db_scan);
  ReviewWOW64Redirect(OldValue_W64b);

  CloseWindow(hwnd);
  PostQuitMessage(0);
}