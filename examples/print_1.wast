(module
    (import "wdb_tui" "prints" (func $print (param i32 i32)))
    (memory 1)
    (data 
        (i32.const 0)
        "Hello World"
    )
    (func $main 
        (export "main")
        (call $print
            (i32.const 0)
            (i32.const 11)
        )
    )
)
