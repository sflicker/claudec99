# ClaudeC99 Stage 95-05 convert medium risk static usages

## Tasks
  - Using the fix-cpacity-inventory.md report as a guide convert the medium-risk static usages
  - Use the Vec and StrBuf modules developed in earlier stages to replace the existing static usages.
  - Additional functionality maybe be added to Vec and StrBuf if necessary.
  - If there is a question of Risk vs Priority from inventory a reanalysis could be done
    But the items converted in this stage should be the ones that are least likely to cause
    bugs/lowest blast radius etc.
  - When finishing the stage the fix-capacity-inventory.md should be explicity updated so 
    follow on stages can pickup what to do next. This should be done at the end when other 
    documenation artifacts are generated after all tests are passing.

## Out of Scope
  - high risk items (these will be done in follow on stages)

## Tests
  - If additional tests are necessary for adequate coverage they should be added
  - If some tests needs to be updated that should be done as well.