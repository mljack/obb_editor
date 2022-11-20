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

(function(app) {
  const textArea = document.getElementById('textEditor');

  // /* Setup the main textarea */
  // textArea.addEventListener('input', () => {

  // });

  // /* Hide menus any time we start typing */
  // textArea.addEventListener('focusin', () => {

  // });



  /**
   * Sets the text of the editor to the specified value
   *
   * @param {string} val
   */
  app.setText = (val) => {
    val = val || '';
    //textArea.value = val;
  };

  /**
   * Gets the text from the editor
   *
   * @return {string}
   */
  app.getText = () => {
    return "";
    //return textArea.value;
  };



})(app);
