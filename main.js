const { app, BrowserWindow, ipcMain, shell, globalShortcut, screen } = require("electron");
const path = require("path");
const { execFile } = require("child_process");

let mainWindow = null;

function getTargetDisplay() {
  const displays = screen.getAllDisplays();

  // Monitor 2 normalmente é o índice 1.
  // Se não existir monitor 2, usa o monitor principal.
  return displays[1] || screen.getPrimaryDisplay();
}

function createWindow() {
  const targetDisplay = getTargetDisplay();
  const { x, y, width, height } = targetDisplay.bounds;

  mainWindow = new BrowserWindow({
    x,
    y,
    width,
    height,
    backgroundColor: "#030508",
    autoHideMenuBar: true,
    fullscreen: true,
    kiosk: true,
    show: false,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false
    }
  });

  mainWindow.loadFile("index.html");

  mainWindow.once("ready-to-show", () => {
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.setBounds({
        x,
        y,
        width,
        height
      });

      mainWindow.show();
      mainWindow.setFullScreen(true);
      mainWindow.setKiosk(true);
    }
  });

  mainWindow.on("leave-full-screen", () => {
    setTimeout(() => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        const currentTargetDisplay = getTargetDisplay();
        const bounds = currentTargetDisplay.bounds;

        mainWindow.setBounds({
          x: bounds.x,
          y: bounds.y,
          width: bounds.width,
          height: bounds.height
        });

        mainWindow.setFullScreen(true);
        mainWindow.setKiosk(true);
      }
    }, 300);
  });
}

app.whenReady().then(() => {
  createWindow();

  globalShortcut.register("CommandOrControl+Shift+Q", () => {
    app.quit();
  });

  globalShortcut.register("F11", () => {
    if (mainWindow && !mainWindow.isDestroyed()) {
      const targetDisplay = getTargetDisplay();
      const { x, y, width, height } = targetDisplay.bounds;

      mainWindow.setBounds({
        x,
        y,
        width,
        height
      });

      mainWindow.setFullScreen(true);
      mainWindow.setKiosk(true);
    }
  });

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("will-quit", () => {
  globalShortcut.unregisterAll();
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

function runPowerShell(script) {
  return new Promise((resolve, reject) => {
    execFile(
      "powershell.exe",
      [
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-Command",
        script
      ],
      {
        windowsHide: true,
        timeout: 8000
      },
      (error, stdout, stderr) => {
        if (error) {
          reject(error);
          return;
        }

        resolve(String(stdout || "").trim());
      }
    );
  });
}

async function sendWindowsMediaKey(vkCode) {
  const script = `
    Add-Type -TypeDefinition @"
    using System;
    using System.Runtime.InteropServices;

    public class Keyboard {
      [DllImport("user32.dll", SetLastError = true)]
      public static extern void keybd_event(byte bVk, byte bScan, int dwFlags, int dwExtraInfo);
    }
"@

    [Keyboard]::keybd_event(${vkCode}, 0, 0, 0)
    Start-Sleep -Milliseconds 60
    [Keyboard]::keybd_event(${vkCode}, 0, 2, 0)
  `;

  await runPowerShell(script);

  return {
    ok: true,
    vkCode
  };
}

function parseSpotifyTitle(windowTitle) {
  const rawTitle = String(windowTitle || "").trim();

  if (!rawTitle) {
    return {
      title: "",
      artist: "",
      rawTitle: "",
      isPlayingLike: false
    };
  }

  const lower = rawTitle.toLowerCase();

  const blockedTitles = [
    "spotify",
    "spotify premium",
    "spotify free"
  ];

  if (blockedTitles.includes(lower)) {
    return {
      title: "Spotify Desktop",
      artist: "Aberto",
      rawTitle,
      isPlayingLike: false
    };
  }

  const separators = [
    " - ",
    " – ",
    " — "
  ];

  for (const separator of separators) {
    if (rawTitle.includes(separator)) {
      const parts = rawTitle
        .split(separator)
        .map((item) => item.trim())
        .filter(Boolean);

      if (parts.length >= 2) {
        return {
          artist: parts[0],
          title: parts.slice(1).join(separator),
          rawTitle,
          isPlayingLike: true
        };
      }
    }
  }

  return {
    title: rawTitle,
    artist: "Spotify Desktop",
    rawTitle,
    isPlayingLike: true
  };
}

async function getSpotifyStatus() {
  try {
    const script = `
      $ErrorActionPreference = "SilentlyContinue"

      $mainProc = Get-Process Spotify |
        Where-Object { $_.MainWindowTitle -and $_.MainWindowTitle.Trim() -ne "" } |
        Select-Object -First 1 Id, ProcessName, MainWindowTitle

      if ($null -eq $mainProc) {
        $anyProc = Get-Process Spotify |
          Select-Object -First 1 Id, ProcessName, MainWindowTitle

        if ($null -eq $anyProc) {
          [PSCustomObject]@{
            isOpen = $false
            processId = $null
            windowTitle = ""
          } | ConvertTo-Json -Compress
        } else {
          [PSCustomObject]@{
            isOpen = $true
            processId = $anyProc.Id
            windowTitle = $anyProc.MainWindowTitle
          } | ConvertTo-Json -Compress
        }
      } else {
        [PSCustomObject]@{
          isOpen = $true
          processId = $mainProc.Id
          windowTitle = $mainProc.MainWindowTitle
        } | ConvertTo-Json -Compress
      }
    `;

    const output = await runPowerShell(script);

    if (!output) {
      return {
        ok: true,
        isOpen: false,
        processId: null,
        windowTitle: "",
        title: "Spotify fechado",
        artist: "Clique em Abrir Spotify",
        rawTitle: "",
        isPlayingLike: false
      };
    }

    const parsed = JSON.parse(output);
    const parsedTitle = parseSpotifyTitle(parsed.windowTitle);

    return {
      ok: true,
      isOpen: Boolean(parsed.isOpen),
      processId: parsed.processId || null,
      windowTitle: parsed.windowTitle || "",
      title: parsedTitle.title,
      artist: parsedTitle.artist,
      rawTitle: parsedTitle.rawTitle,
      isPlayingLike: parsedTitle.isPlayingLike
    };
  } catch (error) {
    return {
      ok: false,
      isOpen: false,
      processId: null,
      windowTitle: "",
      title: "Erro no Spotify",
      artist: error.message,
      rawTitle: "",
      isPlayingLike: false
    };
  }
}

ipcMain.handle("media:control", async (_event, action) => {
  const normalizedAction = String(action || "").toLowerCase();

  const keyMap = {
    toggle: 0xB3,
    playpause: 0xB3,
    play: 0xB3,
    pause: 0xB3,
    next: 0xB0,
    previous: 0xB1,
    prev: 0xB1,
    stop: 0xB2
  };

  const vkCode = keyMap[normalizedAction];

  if (!vkCode) {
    return {
      ok: false,
      status: false,
      message: `Ação inválida: ${action}`
    };
  }

  await sendWindowsMediaKey(vkCode);

  return {
    ok: true,
    status: true,
    action: normalizedAction,
    vkCode
  };
});

ipcMain.handle("media:status", async () => {
  return await getSpotifyStatus();
});

ipcMain.handle("spotify:open", async () => {
  await shell.openExternal("spotify:");

  return {
    ok: true
  };
});