; Inno Setup скрипт для создания установщика Voice Upside Down
; Требуется: Inno Setup Compiler (https://jrsoftware.org/isdl.php)

#define MyAppName "Voice Upside Down"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "VoiceUpsideDown"
#define MyAppURL "https://voiceupside.local"
#define MyAppExeName "voice_upside_down.exe"

[Setup]
; Основные настройки
AppId={{A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=.
OutputBaseFilename=VoiceUpsideDown_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1

[Files]
; Исполняемый файл
Source: "build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src\voice_upside_down.exe"; DestDir: "{app}"; Flags: ignoreversion
; Все DLL из директории сборки (должны быть собраны через windeployqt)
Source: "build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src\*.dll"; DestDir: "{app}"; Flags: ignoreversion
; QML плагины и модули
Source: "build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src\qml\*"; DestDir: "{app}\qml"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src\plugins\*"; DestDir: "{app}\plugins"; Flags: ignoreversion recursesubdirs createallsubdirs
; Дополнительные файлы, если есть
Source: "build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs
; Visual C++ Redistributable (опционально, если включен)
; Source: "redist\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
; Опционально: запуск Visual C++ Redistributable
; Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Установка Visual C++ Redistributable..."; Check: VCRedistNeedsInstall
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// Функция для проверки необходимости установки VC++ Redistributable
function VCRedistNeedsInstall: Boolean;
var
  Version: String;
begin
  Result := not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', Version);
end;

