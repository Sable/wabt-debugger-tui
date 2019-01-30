(module
    (memory 1)
    (func $add
        (export "add")
        (param $a i32)
        (param $b i32)
        (result i32)
        (i32.add
            (get_local $a)
            (get_local $b)
        )
    )
    (func $add_1
        (export "add_1")
        (result i32)
        (call $add
            (i32.const 1)
            (i32.const 2)
        )
    )
    (func $add_2
        (export "add_2")
        (result i32)
        (call $add
            (i32.const 2)
            (i32.const 3)
        )
    )
    (func $add_3
        (export "add_3")
        (result i32)
        (call $add
            (i32.const 3)
            (i32.const 4)
        )
    )
)
