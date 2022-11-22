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
if (!app.hasFSAccess) {
  alert("Please use Chrome browser.");
}

app.files = {}

app.load_file = async (file_path) => {
  const file = await app.files[file_path].getFile();
  return await readFile(file);
}

app.save_file = async (file_path, content) => {
  try {
    if (!(file_path in app.files)) {
      let fileHandle;
      try {
        fileHandle = await getNewFileHandle(file_path);
        app.files[file_path] = fileHandle;
      } catch (ex) {
        if (ex.name === 'AbortError') {
          return;
        }
        const msg = 'An error occured trying to open the file.';
        console.error(msg, ex);
        alert(msg);
        return;
      }
    }
    await writeFile(app.files[file_path], content);
  } catch (ex) {
    const msg = 'Unable to save file';
    console.error(msg, ex);
    alert(msg);
  }
};

app.file_type_exts = new Set(["jpg", "jpeg", "png", "bmp"]);
app.file_type_exts2 = new Set(["json"]);
app.traverse_subfolders = false;

app.recursive_scan_folder = async (folder_handle, path_id) => {
  var path_c_str = _malloc(1024);
  for await (const handle of folder_handle.values()) {
    //console.log("------------------------\n")
    const full_path = path_id + "/" + handle.name
    console.log(handle.kind, full_path);
    if (handle.kind == "directory") {
      if (app.traverse_subfolders) {
        stringToUTF8Array(handle.name, HEAP8, path_c_str, handle.name.length+1);
        Module._add_folder(path_c_str);
        await app.recursive_scan_folder(handle, full_path);
        continue
      }
    }
    const ext = handle.name.split(".").pop().toLowerCase();
    if (app.file_type_exts2.has(ext))
      app.files[full_path] = handle;
    else if (app.file_type_exts.has(ext)) {
      //console.log("before add_file()", handle.name);
      stringToUTF8Array(full_path, HEAP8, path_c_str, full_path.length+1);
      Module._add_file(path_c_str);
      //console.log("after add_file()");
      app.files[full_path] = handle;
    }
  }
  Module._pop_folder();
  _free(path_c_str)
}

app.open_folder = async () => {
    try {
      var folder_handle = await getFolderHandle();
    } catch (ex) {
      if (ex.name === 'AbortError') {
        return;
      }
      const msg = 'An error occured trying to open the file.';
      console.error(msg, ex);
      alert(msg);
    }

  if (!folder_handle)
    return;

  Module._new_filelist();
  app.recursive_scan_folder(folder_handle, ".");
};

