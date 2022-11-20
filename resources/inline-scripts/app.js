/**
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

/* globals getFileHandle, getNewFileHandle, readFile, verifyPermission,
           writeFile */

// eslint-disable-next-line no-redeclare
const app = {
  appName: 'Text Editor',
  file: {
    handle: null,
    name: null,
    isModified: false,
  },
  options: {
    captureTabs: true,
    fontSize: 14,
    monoSpace: false,
    wordWrap: true,
  },
  hasFSAccess: 'chooseFileSystemEntries' in window ||
               'showOpenFilePicker' in window,
  isMac: navigator.userAgent.includes('Mac OS X'),
};

// Verify the APIs we need are supported, show a polite warning if not.
if (app.hasFSAccess) {
  //document.getElementById('not-supported').classList.add('hidden');
} else {
  document.getElementById('lblLegacyFS').classList.toggle('hidden', false);
  document.getElementById('butSave').classList.toggle('hidden', true);
}

/**
 * Creates an empty notepad with no details in it.
 */
app.newFile = () => {

  app.setText();
  app.setFile();
};

app.setFile = (fileHandle) => {
  if (fileHandle && fileHandle.name) {
    app.file.handle = fileHandle;
    app.file.name = fileHandle.name;
    document.title = `${fileHandle.name} - ${app.appName}`;
  } else {
    app.file.handle = null;
    app.file.name = fileHandle;
    document.title = app.appName;
  }
};

/**
 * Opens a file for reading.
 *
 * @param {FileSystemFileHandle} fileHandle File handle to read from.
 */
app.openFile = async (fileHandle) => {
  // If the File System Access API is not supported, use the legacy file apis.
  if (!app.hasFSAccess) {
    const file = await app.getFileLegacy();
    if (file) {
      app.readFile(file);
    }
    return;
  }

  // If a fileHandle is provided, verify we have permission to read/write it,
  // otherwise, show the file open prompt and allow the user to select the file.
  if (fileHandle) {
    if (await verifyPermission(fileHandle, true) === false) {
      console.error(`User did not grant permission to '${fileHandle.name}'`);
      return;
    }
  } else {
    try {
      fileHandle = await getFileHandle();
    } catch (ex) {
      if (ex.name === 'AbortError') {
        return;
      }
      const msg = 'An error occured trying to open the file.';
      console.error(msg, ex);
      alert(msg);
    }
  }

  if (!fileHandle) {
    return;
  }
  const file = await fileHandle.getFile();
  app.readFile(file, fileHandle);
};


app.files = {}

app.load_file = async (file_path) => {
  const file = await app.files[file_path].getFile();
  return await readFile(file);
}

app.openFolder = async () => {
    try {
      var folderHandle = await getFolderHandle();
    } catch (ex) {
      if (ex.name === 'AbortError') {
        return;
      }
      const msg = 'An error occured trying to open the file.';
      console.error(msg, ex);
      alert(msg);
    }

  if (!folderHandle)
    return;

  const file_type_exts = new Set(["jpg", "jpeg", "png", "bmp"]);
  
  Module._new_filelist()
  var path = _malloc(1024);
  for await (const fileHandle of folderHandle.values()) {
    console.log("------------------------\n")
    console.log(fileHandle.kind, fileHandle.name);
    if (fileHandle.kind !== 'file')
      continue;
    const ext = fileHandle.name.split(".").pop().toLowerCase();
    if (!file_type_exts.has(ext))
      continue;
    stringToUTF8Array(fileHandle.name, HEAP8, path, fileHandle.name.length+1);
    Module._add_file(path)
    //var filePromise = fileHandle.getFile().then((file) => file.text());
    //console.log(await filePromise);

    //var file = await fileHandle.getFile();
    //var fileHandle2 = fileHandle;
    app.files[fileHandle.name] = fileHandle;
  }
  _free(path)
  //app.readFile(file, fileHandle2);
};


app.openFolderFile = async (idx) => {
  try {
    var fileHandle = app.files[idx];
    var file = await fileHandle.getFile()
    //console.info(await readFile(file))
    app.readFile(file, fileHandle)
  } catch (ex) {
    const msg = `An error occured reading ${app.fileName}`;
    console.error(msg, ex);
    alert(msg);
  }
};

app.printFile = async (file) => {
  try {
    console.info(await readFile(file))
  } catch (ex) {
    const msg = `An error occured reading ${app.fileName}`;
    console.error(msg, ex);
    alert(msg);
  }
};

/**
 * Read the file from disk.
 *
 *  @param {File} file File to read from.
 *  @param {FileSystemFileHandle} fileHandle File handle to read from.
 */
app.readFile = async (file, fileHandle) => {
  try {
    app.setText(await readFile(file));
    app.setFile(fileHandle || file.name);
    //console.log(app.getText())
  } catch (ex) {
    const msg = `An error occured reading ${app.fileName}`;
    console.error(msg, ex);
    alert(msg);
  }
};

/**
 * Saves a file to disk.
 */
app.saveFile = async () => {
  try {
    if (!app.file.handle) {
      return await app.saveFileAs();
    }
    await writeFile(app.file.handle, app.getText());
  } catch (ex) {
    const msg = 'Unable to save file';
    console.error(msg, ex);
    alert(msg);
  }
};

/**
 * Saves a new file to disk.
 */
app.saveFileAs = async () => {
  if (!app.hasFSAccess) {
    app.saveAsLegacy(app.file.name, app.getText());
    return;
  }
  let fileHandle;
  try {
    fileHandle = await getNewFileHandle();
  } catch (ex) {
    if (ex.name === 'AbortError') {
      return;
    }
    const msg = 'An error occured trying to open the file.';
    console.error(msg, ex);
    alert(msg);
    return;
  }
  try {
    await writeFile(fileHandle, app.getText());
    app.setFile(fileHandle);
  } catch (ex) {
    const msg = 'Unable to save file.';
    console.error(msg, ex);
    alert(msg);
    return;
  }
};

/**
 * Attempts to close the window
 */
app.quitApp = () => {

  window.close();
};
