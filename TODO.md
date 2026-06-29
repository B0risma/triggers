## todo list  

### Needs  
 - API
    - trigger list (event sources) - V
    - Target list (relised as Command range) - V
    - Command ctrl (as action sublist) - V
    - Actions
        - Create - V
        - Del - V
        - List - V
        - Range - ? how to do it?
    - Action links (circuit\link) - V - need updating
 - Integration () - X
    - product based events
    - real commands

 - tests - X
    - moc tests - X
     - adding triggers (some triggers)
     - creating actions with commands
     - link 
     - event 
    - vswitch - V

### todo  

1. check event routes (composite key or source only)
1. Targets dont subscribe to queue after addition to TargetList - (use target list only inside queue and add targets with queue) OR ( use external targ list and auto subcribe while addition) AND NO API for target list
1.  Complete API 
    - Action range - LATER
    - make supported rule type list (as rule key or just rule type == target type)
1. Strict trigger (event) type types (trigger key as event key)
1. Tests 
1. Final integration
