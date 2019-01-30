(module
    (memory 1)
    (func $main 
        (export "main")
        (result i32)
        (local $i i32)
        (loop $loop_1
            (br_if $loop_1
                (i32.ne 
                    (tee_local $i
                        (i32.add 
                            (get_local $i)
                            (i32.const 2)
                        )
                    )
                    (i32.const 100)
                )
            )
        )
        (get_local $i)
    )
)
