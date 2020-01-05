# mare-interpreter

## info
- m-interpreter (mare-interpreter) : mare script ����(*.mre)�� �ؼ��Ͽ� ������.

### build : m-interpreter (for mac, ubuntu)

- build ���� : build.sh 

### usage : m-interpreter
- ./m-interpreter script-filename
    - process: script load, verify(syntax), run(func), save(memory for next run)
    - script ���� ������ ���� ��, main function (���� ���, ó������) �����ѵ�, ���� ������ ���� ������ ����
- ./m-interpreter create mare-script-filename func_name func_arges...
    - process: script load, verify(syntax), run(func), save(memory for next run)
    - script ���� ������ ���� ��, �־��� function�� �����ѵ�, ���� ������ ���� ������ ����
- ./m-interpreter run func_name func_arges...
    - process: load(from saved file), run(func), save(updated memory)
    - ����� ������ ������ ���� ��, �־��� function�� �����ѵ�, ���� ������ ���� ������ ����

