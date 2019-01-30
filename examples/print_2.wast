(module
    (import "wdb_tui" "prints" (func $print (param i32 i32)))
    (func $main 
        (export "main")
        (call $print
            (i32.const 0)
            (i32.const 11)
        )
    )
)
