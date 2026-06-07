# ClaudeC99 Stage 95-07 convert high risk static usages

## Tasks
  - Using the fix-cpacity-inventory.md report as a guide convert the remaining static usages
  - Use the Vec and StrBuf modules developed in earlier stages to replace the existing static usages.
  - Additional functionality maybe be added to Vec and StrBuf if necessary.
  - If there is a question of Risk vs Priority from inventory a reanalysis could be done
    But the items converted in this stage should be the ones that are least likely to cause
    bugs/lowest blast radius etc.
  - When finishing the stage the fix-capacity-inventory.md should be explicity updated so 
    follow on stages can pickup what to do next. This should be done at the end when other 
    documenation artifacts are generated after all tests are passing.

## Notes
These converions have taken quite a long time on previous stages for the coding agent
to fix bugs that pop up. There are some patterns to watch out for in the fix-capacity-inventory.md.
Also look at the last few stages artififacts.

Do conversions one at a time. Make a checklist of conversions that will be 
performed in this stage. For each conversions update the primary code and all potentially related
code. Then run either the full test suite a revelvant port of it. once the tests are passing
commit the changes/update the checklist then repeat.

## Tests
  - If additional tests are necessary for adequate coverage they should be added
  - If some tests needs to be updated that should be done as well.