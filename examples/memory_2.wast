(module
    (memory 1)
    (func $main
        (export "main")
        (i32.store
            (i32.const 0)
            (i32.const 42)
        )
        (i64.store
            (i32.const 16)
            (i64.const 120)
        )
        (f32.store
            (i32.const 32)
            (f32.const 1.1)
        )
        (f64.store
            (i32.const 48)
            (f64.const 2.2)
        )
    )
)
