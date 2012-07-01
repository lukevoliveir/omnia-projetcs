//------------------------------------------------------------------------------
// Projet RtCA          : Read to Catch All
// Auteur               : Nicolas Hanteville
// Site                 : http://code.google.com/p/omnia-projetcs/
// Licence              : GPL V3
//------------------------------------------------------------------------------
#include "../RtCA.h"
//------------------------------------------------------------------------------
void addPasswordtoDB(char *source, char*login, char*password, char*raw_password,unsigned int description_id,unsigned int session_id, sqlite3 *db)
{
  char request[REQUEST_MAX_SIZE];
  snprintf(request,REQUEST_MAX_SIZE,
           "INSERT INTO extract_registry_account_password (source,login,password,raw_password,description_id,session_id) "
           "VALUES(\"%s\",\"%s\",\"%s\",\"%s\",%d,%d);",
           source,login,password,raw_password,description_id,session_id);
  if (!CONSOL_ONLY || DEBUG_CMD_MODE)AddDebugMessage("test_registry_password", request, "-", MSG_INFO);
  sqlite3_exec(db,request, NULL, NULL, NULL);
}
//------------------------------------------------------------------------------
void vncpwd(unsigned char *pwd, int bytelen)
{     // if bytelen is 0 it's a hex string
  int     len,
          tmp;
  unsigned char  fixedkey[8] = { 23,82,107,6,35,78,88,7 },
          *p,
          *o;

  if(bytelen)
  {
      o = pwd + bytelen;
  } else {
      for(p = o = pwd; *p; p += 2, o++)
      {
          sscanf(p, "%02x", &tmp);
          *o = tmp;
      }
  }

  len = o - pwd;
  tmp = len % 8;
  len /= 8;
  if(tmp) len++;

  deskey(fixedkey, 1);
  for(p = pwd; len--; p += 8) des(p, p);

  *o = 0;
}
//------------------------------------------------------------------------------
//local function part !!!
//------------------------------------------------------------------------------
void Scan_registry_password_local(sqlite3 *db,unsigned int session_id)
{
  char login[MAX_PATH], password[MAX_PATH], raw_password[MAX_PATH],tmp[MAX_PATH];

  //--------------------------------------------------
  //read login and password of application in registry
  //--------------------------------------------------
  #define REG_PASSWORD_STRING_VNC                0x600
  #define REG_PASSWORD_STRING_SCREENSAVER        0x601
  #define REG_PASSWORD_STRING_TERMINAL_SERVER    0x602
  #define REG_PASSWORD_STRING_AUTO_LOGON         0x603

  //VNC3
  login[0] = 0;
  password[0] = 0;
  raw_password[0] = 0;

  if (ReadValue(HKEY_CURRENT_USER,"SOFTWARE\\ORL\\WinVNC3","Password",password, MAX_PATH) == FALSE)
    if (ReadValue(HKEY_CURRENT_USER,"SOFTWARE\\ORL\\WinVNC3\\Default","Password",password, MAX_PATH) == FALSE)
      if (ReadValue(HKEY_LOCAL_MACHINE,"SOFTWARE\\ORL\\WinVNC3","Password",password, MAX_PATH) == FALSE)
        ReadValue(HKEY_LOCAL_MACHINE,"SOFTWARE\\ORL\\WinVNC3\\Default","Password",password, MAX_PATH);

  if (strlen(password)>3)
  {
    snprintf(raw_password,MAX_PATH,"0x%02x%02x%02x%02x",password[0]&255,password[1]&255,password[2]&255,password[3]&255);
    vncpwd((u_char *)password,8);
    addPasswordtoDB("Registry\\VNC",login,password,raw_password,REG_PASSWORD_STRING_VNC,session_id,db);
  }

  //Screen saver
  login[0] = 0;
  password[0] = 0;
  raw_password[0] = 0;
  tmp[0] = 0;

  if (ReadValue(HKEY_CURRENT_USER,".DEFAULT\\CONTROL PANEL\\DESKTOP","ScreenSave_Data",tmp, MAX_PATH) == FALSE)
    ReadValue(HKEY_USERS,".DEFAULT\\CONTROL PANEL\\DESKTOP","ScreenSave_Data",tmp, MAX_PATH);

  if (tmp[0] != 0)
  {
    int i,j=0,k=0,size = strlen(tmp);
    if (size>0)
    {
      if (size>0xFF)size=0xFF;

      //permet d�chiffrement (codage XOR avec la cl� de windows)
      const BYTE key[128] = {
      0x48, 0xEE, 0x76, 0x1D, 0x67, 0x69, 0xA1, 0x1B, 0x7A, 0x8C, 0x47, 0xF8, 0x54, 0x95, 0x97, 0x5F,
      0x78, 0xD9, 0xDA, 0x6C, 0x59, 0xD7, 0x6B, 0x35, 0xC5, 0x77, 0x85, 0x18, 0x2A, 0x0E, 0x52, 0xFF,
      0x00, 0xE3, 0x1B, 0x71, 0x8D, 0x34, 0x63, 0xEB, 0x91, 0xC3, 0x24, 0x0F, 0xB7, 0xC2, 0xF8, 0xE3,
      0xB6, 0x54, 0x4C, 0x35, 0x54, 0xE7, 0xC9, 0x49, 0x28, 0xA3, 0x85, 0x11, 0x0B, 0x2C, 0x68, 0xFB,
      0xEE, 0x7D, 0xF6, 0x6C, 0xE3, 0x9C, 0x2D, 0xE4, 0x72, 0xC3, 0xBB, 0x85, 0x1A, 0x12, 0x3C, 0x32,
      0xE3, 0x6B, 0x4F, 0x4D, 0xF4, 0xA9, 0x24, 0xC8, 0xFA, 0x78, 0xAD, 0x23, 0xA1, 0xE4, 0x6D, 0x9A,
      0x04, 0xCE, 0x2B, 0xC5, 0xB6, 0xC5, 0xEF, 0x93, 0x5C, 0xA8, 0x85, 0x2B, 0x41, 0x37, 0x72, 0xFA,
      0x57, 0x45, 0x41, 0xA1, 0x20, 0x4F, 0x80, 0xB3, 0xD5, 0x23, 0x02, 0x64, 0x3F, 0x6C, 0xF1, 0x0F};

      strcpy(raw_password,tmp);
      for (i=0; i<size/2; i++)
      {
        k = 0;
        // transformer en entier
        j -= (j=tmp[i*2  ])>0x39 ? 0x37:0x30;
        k += j*0x10;
        j -= (j=tmp[i*2+1])>0x39 ? 0x37:0x30;
        k += j;
        // effectuer le d�cryptage
        k ^= key[i];
        tmp[i] = k;
      }
      tmp[size/2] = 0;
      strcpy(password,tmp);

      addPasswordtoDB("Registry\\Screen saver",login,password,raw_password,REG_PASSWORD_STRING_SCREENSAVER,session_id,db);
    }
  }

  //terminal server
  login[0] = 0;
  password[0] = 0;
  raw_password[0] = 0;

  if (ReadValue(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\DefaultUserConfiguration","password",raw_password, MAX_PATH) == FALSE)
    if (ReadValue(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\Console","password",raw_password, MAX_PATH) == FALSE)
      ReadValue(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp","password",raw_password, MAX_PATH);

  if (raw_password[0] != 0)
  {
    addPasswordtoDB("Registry\\Terminal server",login,raw_password,raw_password,REG_PASSWORD_STRING_TERMINAL_SERVER,session_id,db);
  }

  //Session auto login
  tmp[0] = 0;
  login[0] = 0;
  password[0] = 0;
  raw_password[0] = 0;

  ReadValue(HKEY_LOCAL_MACHINE,"Software\\Windows NT\\CurrentVersion\\Winlogon","DefaultUserName",password, MAX_PATH);
  ReadValue(HKEY_LOCAL_MACHINE,"Software\\Windows NT\\CurrentVersion\\Winlogon","DefaultPassword",raw_password, MAX_PATH);
  ReadValue(HKEY_LOCAL_MACHINE,"Software\\Windows NT\\CurrentVersion\\Winlogon","DefaultDomainName",tmp, MAX_PATH);

  if (tmp[0] != 0 || password[0] != 0 || raw_password[0] != 0)
  {
    snprintf(login,MAX_PATH,"%s\\%s",tmp,password); //domaine\\user
    addPasswordtoDB("Registry\\Windows Auto-logon",login,raw_password,raw_password,REG_PASSWORD_STRING_AUTO_LOGON,session_id,db);
  }

  //--------------------------------------------------
  //read login+password of user in registry
  //--------------------------------------------------

  //--------------------------------------------------
  //read login+password of AD
  //--------------------------------------------------

  //--------------------------------------------------
  //read login+password of cachedump
  //--------------------------------------------------

}
//------------------------------------------------------------------------------
//file registry part
//------------------------------------------------------------------------------
void Scan_registry_password_file(HK_F_OPEN *hks,sqlite3 *db,unsigned int session_id)
{

}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DWORD WINAPI Scan_registry_password(LPVOID lParam)
{
  //init
  sqlite3 *db = (sqlite3 *)db_scan;
  unsigned int session_id = current_session_id;
  WaitForSingleObject(hsemaphore,INFINITE);
  AddDebugMessage("test_registry_password", "Scan registry passwords - START", "OK", MSG_INFO);

  char file[MAX_PATH];
  char tmp_msg[MAX_PATH];
  HK_F_OPEN hks;

  //files or local
  HTREEITEM hitem = (HTREEITEM)SendDlgItemMessage((HWND)h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_CHILD, (LPARAM)TRV_HTREEITEM_CONF[FILES_TITLE_REGISTRY]);
  if (hitem!=NULL) //files
  {
    while(hitem!=NULL)
    {
      file[0] = 0;
      GetTextFromTrv(hitem, file, MAX_PATH);
      if (file[0] != 0)
      {
        //info
        snprintf(tmp_msg,MAX_PATH,"Scan Registry file : %s",file);
        AddDebugMessage("test_registry_password", tmp_msg, "OK", MSG_INFO);

        //open file + verify
        if(OpenRegFiletoMem(&hks, file))
        {
          Scan_registry_password_file(&hks, db, session_id);

          CloseRegFiletoMem(&hks);
        }
      }
      hitem = (HTREEITEM)SendDlgItemMessage((HWND)h_conf,TRV_FILES, TVM_GETNEXTITEM,(WPARAM)TVGN_NEXT, (LPARAM)hitem);
    }
  }else Scan_registry_password_local(db,session_id);

  AddDebugMessage("test_registry_password", "Scan registry passwords  - DONE", "OK", MSG_INFO);
  check_treeview(GetDlgItem(h_conf,TRV_TEST), H_tests[(unsigned int)lParam], TRV_STATE_UNCHECK);//db_scan
  ReleaseSemaphore(hsemaphore,1,NULL);
  return 0;
}
