const { contextBridge, ipcRenderer } = require("electron");

const allowedChannels = [
  "media:control",
  "media:status",
  "spotify:open"
];

contextBridge.exposeInMainWorld("electronAPI", {
  mediaControl: (action) => ipcRenderer.invoke("media:control", action),

  getMediaStatus: () => ipcRenderer.invoke("media:status"),

  openSpotify: () => ipcRenderer.invoke("spotify:open"),

  invoke: (channel, ...args) => {
    if (!allowedChannels.includes(channel)) {
      throw new Error(`Canal IPC bloqueado: ${channel}`);
    }

    return ipcRenderer.invoke(channel, ...args);
  }
});