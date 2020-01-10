# mare-interpreter

## info
- m-interpreter (mare-interpreter) : mare script 파일(*.mre)을 해석하여 실행함.

### build : m-interpreter (for mac, ubuntu)

- build 참조 : build.sh 

### usage : m-interpreter
- ./m-interpreter script-filename
    - process: script load, verify(syntax), run(func), save(memory for next run)
    - script 원본 파일을 읽은 후, main function (없을 경우, 처음부터) 실행한뒤, 다음 실행을 위한 데이터 저장
- ./m-interpreter create mare-script-filename func_name func_arges...
    - process: script load, verify(syntax), run(func), save(memory for next run)
    - script 원본 파일을 읽은 후, 주어진 function을 실행한뒤, 다음 실행을 위한 데이터 저장
- ./m-interpreter run func_name func_arges...
    - process: load(from saved file), run(func), save(updated memory)
    - 저장된 데이터 파일을 읽은 후, 주어진 function을 실행한뒤, 다음 실행을 위한 데이터 저장

