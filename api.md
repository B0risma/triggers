## RULES
base url: http://localhost:8080/rule

### get policies
GET http://localhost:8080/rule

responce example:
```json
{
  "23435": {
    "rule": {
      "name": "test",
      "type": "sw"
    },
    "targets": [
      "1",
      "3"
    ],
    "type": "policy"
  }
}
```

### add policy
example:  
PATCH http://localhost:8080/rule?name=test1
```json
{
  "type":"policy", // unused
  "rule" : {
      "type" :"sw", // rule type - switch
      "enabled" : false, // unused for switch
      "name" : "test" // switch source name 
  },
  "targets" : ["1","3"] //"module" name
}
```

### set policies
POST http://localhost:8080/rule
req example
```json
{
  "23435": {
    "rule": {
      "name": "test",
      "type": "sw"
    },
    "targets": [
      "1",
      "3"
    ],
    "type": "policy"
  }
}
```


