# Contributing to `Signaletic`

Thanks for contributing to this project. Please read through this page to help you understand what to expect from the process.

## Code of Conduct

This project is governed by the [Signaletic Code of Conduct](CODE_OF_CONDUCT.md). All contributors are expected to respect these social practices.

## Process/Workflow

To contribute a bug report or feature request:

1. Search [our issues](https://github.com/continuing-creativity/signaletic/issues) to confirm that the issue hasn't already been reported.
2. Create a new bug report, following the guidance in the issue template.
3. Discuss the bug report in the ticket and in community forums.

If you would like to contribute a code or documentation change to address an issue or add new features or functionality:

1. First file an issue in the [project's issue tracker](https://github.com/continuing-creativity/signaletic/issues) as outlined above. This should describe the nature of work, what's involved in implementing it, and any questions or challenges that you're aware of in the task.
2. Fork the project repository.
3. Create a branch based on the latest code in the `main` branch.
4. Make the changes described in the associated ticket (see "Coding Guidelines" below).
5. Submit a pull request against the project repository's `main` branch.  If the pull request is meant to resolve a known issue, include text like "Resolves #18", "Fixes #28" in the pull request title and in appropriate commit messages.
6. Work with reviewers to discuss your changes and address any feedback.

### Coding Guidelines

In general, pull requests should:

1. Provide meaningful commit messages (see below).
2. Include tests verifying the changes (see below).
4. Update or add markdown documentation for API changes.
5. Provide documentation for new functions.
6. Contain only your own original creative work, or the work of other authors who are clearly attributed (with links) that is shared under a compatible open source license (please ask if in doubt)

#### Commit Messages

All commit log messages should include the following information:

1. A reference to the GitHub issue this commit applies to (at the beginning of the first line).
2. A meaningful, but short description of the change.

A good commit message might look like:

```shell
commit -am "gh-12: Initial implementation of neutron flow reversal."
commit -am "gh-12: Adds documentation based on PR feedback."
```

#### Tests
The tests for this package are currently written using
[Unity](http://www.throwtheswitch.org/unity), and can be run for both native and Web Assembly builds. See the [project README](README.md) for more details.

#### Lint Your Code

Signaletic does not yet include any automated lint checks. In the meantime, here are some basic stylistic guidelines:

1. Use four spaces for indentation
2. Avoid platform-varying types (such as ```short``` or ```long```) in favour of the C99 fixed-width types (e.g. ```int16_t``` and ```int32_t```)
3. Pointers should be defined as ```float* foo``` rather than ```float *foo```
4. Avoid unnecessary use of ```typedef```. Use ```struct foo {}``` instead of ```typedef struct {} foo;```
5. All definitions should be namespaced. Core Signaletic functions, structs, enums, variable definitions, etc. begin with the prefix ```sig_```.
6. Provide a ```struct sig_Status``` as the last argument to any function that may cause errors

Stylistic conventions very often raise [bike shed](https://www.bikeshed.org/) issues for communities. While these may not be your preferred choices, hopefully we can all agree to disagree. If you find that you are having trouble satisfying one or more conventions, or if you think a stylistic change is warranted, please feel free to raise it in a way that is respectful of the sometimes distracting and polarizing nature of such debates.
