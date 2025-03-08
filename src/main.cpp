#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <set>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

// 제외할 경로 리스트
std::vector<std::string> excludedPaths = {
    //"C:\\Windows\\System32\\",
    //"C:\\Program Files\\Common Files\\"
};

// 특정 경로의 DLL을 제외하는 필터 함수
bool isExcludedDll(const std::string& dllPath) {
    return std::any_of(excludedPaths.begin(), excludedPaths.end(), [&](const std::string& path) {
        return dllPath.find(path) != std::string::npos;
        });
}

// 현재 프로세스의 로드된 DLL 목록을 가져오는 함수
std::set<std::string> getLoadedDlls(HANDLE hProcess) {
    std::set<std::string> dllList;
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char szModName[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName))) {
                std::string dllPath(szModName);

                // 제외할 경로인지 확인하고, 해당 DLL은 리스트에서 제외
                if (!isExcludedDll(dllPath)) {
                    dllList.insert(dllPath);
                }
            }
        }
    }
    return dllList;
}

int main() {
    // 현재 프로세스 핸들 가져오기
    HANDLE hProcess = GetCurrentProcess();

    // 초기 DLL 리스트 수집
    std::cout << "[INFO] 초기 DLL 리스트를 수집 중..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::set<std::string> initialDlls = getLoadedDlls(hProcess);
    std::cout << "[INFO] 초기 DLL 개수: " << initialDlls.size() << std::endl;

    // 지속적인 DLL 모니터링
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 2초마다 검사

        std::set<std::string> currentDlls = getLoadedDlls(hProcess);
        std::set<std::string> newDlls;

        // 새로운 DLL 찾기
        for (const auto& dll : currentDlls) {
            if (initialDlls.find(dll) == initialDlls.end()) {
                newDlls.insert(dll);
            }
        }

        // 새로운 DLL이 감지되었을 경우 콘솔 출력
        if (!newDlls.empty()) {
            std::cout << "[ALERT] 새로운 DLL 감지!" << std::endl;
            for (const auto& dll : newDlls) {
                std::cout << " -> " << dll << std::endl;
            }
        }

        // 현재 상태를 갱신
        initialDlls = currentDlls;
    }

    return 0;
}
