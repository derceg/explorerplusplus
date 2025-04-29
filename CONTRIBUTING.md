# Pull requests

- When submitting a pull request, only include a single high-level change. Don't include multiple unrelated changes in the same pull request.

- ClangFormat is used to format the code. A `.clang-format` file exists in the root of the repository and Visual Studio has [built-in support](https://learn.microsoft.com/en-us/visualstudio/ide/reference/options-text-editor-c-cpp-formatting?view=vs-2022) for ClangFormat, so the code can be formatted from within the IDE. That should be done before submitting a pull request.

- When adding a new .cpp/.h file, add the following license header at the top:

  ```cpp
  // Copyright (C) Explorer++ Project
  // SPDX-License-Identifier: GPL-3.0-only
  // See LICENSE in the top level directory
  ```

- Don't use Hungarian notation when naming variables. For example, a variable should be named `title` instead of `szTitle`. This should be done even if the change would be inconsistent with the surrounding code.

- Outside of that, new code should follow the style of existing code.

- When fixing a bug or making a non-trivial functional change, `Documentation\User\History.txt` should be updated. That file has a maximum line length of 65 characters, so any new entries that are added should adhere to that.

- When making non-trivial functional changes, aim to update the documentation (stored under `Documentation\User\Sphinx`) as well.

- When feasible, unit tests should be included. Generally speaking, code that interacts with the filesystem or UI is going to be more difficult to test. However, code that's more isolated from the external environment can absolutely be tested. For example, the code that's involved in command line parsing is very testable. Therefore, deciding whether code can be tested requires judgement, but the long-term goal is to test as much of the code as possible. So, tests should be added if at all feasible.

- If tests are appropriate, add them to the `TestExplorer++` project. If testing code in a new file that has been added, a new test file should also be added. That test file should match the name of the original file, but should have `Test` appended to the name. For example, `HistoryModelTest.cpp` tests the code contained within `HistoryModel.cpp`. A test file should also be added if an existing file has been changed and that file doesn't have an associated test file yet.

# Documentation

- Documentation changes are greatly appreciated. Currently, the documentation (stored under `Documentation\User\Sphinx` and hosted on [Read the Docs](https://explorerplusplus.readthedocs.io/en/latest)) is severely out of date. Work needs to be done to update the documentation to reflect the current behavior of Explorer++.

- Targeted documentation updates would also be very helpful. For example, if you notice that a particular documentation page is inaccurate, submitting a pull request for just that page is still beneficial.

# Translations

- To update an existing translation, please sign up to [Crowdin](https://crowdin.com/) and make the desired changes within the [explorerplusplus](https://crowdin.com/project/explorerplusplus) project there.

- To request a new translation, please create a GitHub issue that states the language that you'd like to work on. That's necessary, since adding a new language requires a set of coordinated changes to be made on Crowdin as well in this repository.
