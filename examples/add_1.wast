(module
    (memory 1)
    (func $main 
        (export "main")
        (result i32)
        (i32.add 
            (i32.const 10)
            (i32.const 2)
        )
    )
)
