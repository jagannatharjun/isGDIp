[Defines]
#define AppName "The Witcher 3: Wild Hunt"
#define AppVersion "0.1"
#define Color "$d03a1d"

[SysDefines]
#define IncludeFile(FileName) "Source:" + FileName + "; Flags: dontcopy"

[Setup]
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName=./

[Files]
Source: CallbackCtrl.dll; Flags: dontcopy
Source: gdi.dll; Flags: dontcopy

[CODE]
#define GDIDLLPATH "files:gdi.dll"

type
  ARGB = DWORD;

function DrawRectangle(h : HWND; LineColor: ARGB;startX: integer;startY: integer; width,
       height: integer): integer;
  external 'DrawRectangle@{#GDIDLLPATH} stdcall delayload';
function DrawImage(h : HWND;FileName: PAnsiChar; startX, startY, width, height : integer): integer;
  external 'DrawImage@{#GDIDLLPATH} stdcall delayload';
function DrawCircle(h : HWND; LineColor: ARGB;startX: integer;startY: integer; width,
       height: integer; StartAngle, SweepAngle : Single): integer;
  external 'DrawCircle@{#GDIDLLPATH} stdcall delayload';
function DrawArc(h : HWND; LineColor: ARGB;startX: integer;startY: integer; width,
       height: integer; StartAngle, SweepAngle : Single; PenWidth : integer): integer;
  external 'DrawArc@{#GDIDLLPATH} stdcall delayload';

function GetWidth(id : Integer) : Integer;
  external 'GetWidth@{#GDIDLLPATH} stdcall delayload';
function GetHeight(id : Integer) : Integer;
  external 'GetHeight@{#GDIDLLPATH} stdcall delayload';
function GetIsVisible(id : Integer) : Integer;
  external 'GetIsVisible@{#GDIDLLPATH} stdcall delayload';

procedure SetOnMouseClick(id : Integer; n : LongWord);
  external 'SetOnMouseClick@{#GDIDLLPATH} stdcall delayload';
procedure SetColor(id : Integer; n : DWORD);
  external 'SetColor@{#GDIDLLPATH} stdcall delayload';
procedure SetWidth(id,integer : Integer);
  external 'SetWidth@{#GDIDLLPATH} stdcall delayload';
procedure SetHeight(id,integer : Integer);
  external 'SetHeight@{#GDIDLLPATH} stdcall delayload';
procedure _SetImageFile(id : Integer; const FileName : PAnsiChar);
  external 'SetImageFile@{#GDIDLLPATH} stdcall delayload';
procedure SetIsVisible(id,v : Integer);
  external 'SetIsVisible@{#GDIDLLPATH} stdcall delayload';
procedure SetStartAngle(id : Integer; n : Single);
  external 'SetStartAngle@{#GDIDLLPATH} stdcall delayload';
procedure SetSweepAngle(id : Integer; n : Single);
  external 'SetSweepAngle@{#GDIDLLPATH} stdcall delayload';

procedure  gdishutdown();
  external 'gdishutdown@{#GDIDLLPATH} stdcall delayload';

var
  CircleId, ArcId: Integer;
  ProgressTimer : TTimer;

function Color1(alpha,red,green,blue : Byte): ARGB;
begin
  Result := 0;
  Result := (DWORD(Result) and $00FFFFFF) or ((DWORD(alpha) and $FF) shl 24);
  Result := (DWORD(Result) and $FF00FFFF) or ((DWORD(red) and $FF) shl 16);
  Result := (DWORD(Result) and $FFFF00FF) or ((DWORD(green) and $FF) shl 8);
  Result := (DWORD(Result) and $FFFFFF00) or ((DWORD(blue) and $FF));
end;

function Color2(alpha : Byte; Color : ARGB): ARGB;
var
 r, g, b: byte;
begin
  r:= Byte(Color and $000000FF);
  g:= Byte((Color shr 8) and $000000FF);
  b:= Byte((Color shr 16) and $000000FF);
  Log(FmtMessage('%4,r: %1,g: %2,b: %3',[IntToStr(r),IntToStr(g),IntToStr(b),IntToStr(Color)]));
  Result := Color1(alpha,r,g,b);
end;

procedure DeinitializeSetup();
begin
  gdishutdown;
end;

var
  Progress : integer;

procedure UpdateProgress(Sender :TObject);
begin
  if Progress > 360 then
    Progress := 0;
  Progress := Progress + 5;
  SetSweepAngle(CircleId,Progress);
  SetSweepAngle(ArcId,Progress);
  WizardForm.WelcomePage.Invalidate;
end;

procedure InitializeWizard();
begin
  Progress := 0;

  WizardForm.WizardBitmapImage.Hide;
  CircleId := DrawCircle(WizardForm.WelcomePage.Handle,Color2(255,clRed),36,36,100,100,0,360);
  ArcId := DrawArc(WizardForm.WelcomePage.Handle,Color2(160,clgreen),36,144,100,100,0,360,10);

 
  ProgressTimer := TTimer.Create(WizardForm);
  with ProgressTimer do
  begin
    Enabled := false;
    OnTimer := @UpdateProgress;
    Interval:= 1000;
  end;
end;


