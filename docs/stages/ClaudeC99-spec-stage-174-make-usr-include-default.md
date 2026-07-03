# ClaudeC99 Stage 174 - make --sysinclude default and add --testinclude option

## Task
make --sysinclude the default for cc99 so /usr/include and system specific headers will be used by default when
running on a linux platform. preserve the usage of <project-root>/test/include but with a new --testinclude option.
cc99 should default to using --testinclude when not running on a linux platform.

All test suite tests should use the default (--sysinclude) if possible (currently running on a linux platform). However if 
the platform independant tests are running on a non linux platform or break for other reasons those tests should be updated to use the --testinclude option.