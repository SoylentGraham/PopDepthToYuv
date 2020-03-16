(module
 (type $FUNCSIG$ff (func (param f32) (result f32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 16) "\00@\80\c4\ff")
 (export "memory" (memory $0))
 (export "Min" (func $Min))
 (export "Range" (func $Range))
 (export "RangeClamped" (func $RangeClamped))
 (export "GetChromaIndex" (func $GetChromaIndex))
 (export "Depth16ToYuv" (func $Depth16ToYuv))
 (func $Min (; 0 ;) (param $0 i32) (param $1 i32) (result i32)
  (select
   (get_local $0)
   (get_local $1)
   (i32.lt_s
    (get_local $0)
    (get_local $1)
   )
  )
 )
 (func $Range (; 1 ;) (param $0 f32) (param $1 f32) (param $2 f32) (result f32)
  (f32.div
   (f32.sub
    (get_local $2)
    (get_local $0)
   )
   (f32.sub
    (get_local $1)
    (get_local $0)
   )
  )
 )
 (func $RangeClamped (; 2 ;) (param $0 f32) (param $1 f32) (param $2 f32) (result f32)
  (select
   (f32.const 0)
   (f32.min
    (tee_local $0
     (f32.div
      (f32.sub
       (get_local $2)
       (get_local $0)
      )
      (f32.sub
       (get_local $1)
       (get_local $0)
      )
     )
    )
    (f32.const 1)
   )
   (f32.lt
    (get_local $0)
    (f32.const 0)
   )
  )
 )
 (func $GetChromaIndex (; 3 ;) (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  (i32.add
   (i32.add
    (i32.mul
     (i32.div_s
      (get_local $1)
      (i32.const 4)
     )
     (get_local $2)
    )
    (i32.div_s
     (get_local $0)
     (i32.const 2)
    )
   )
   (i32.and
    (i32.sub
     (i32.const 0)
     (i32.and
      (i32.div_s
       (get_local $1)
       (i32.const 2)
      )
      (i32.const 1)
     )
    )
    (i32.div_s
     (get_local $2)
     (i32.const 2)
    )
   )
  )
 )
 (func $Depth16ToYuv (; 4 ;) (param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 f32)
  (local $11 f32)
  (local $12 f32)
  (set_local $7
   (i32.div_s
    (get_local $2)
    (i32.const 2)
   )
  )
  (set_local $9
   (i32.div_s
    (get_local $3)
    (i32.const 2)
   )
  )
  (block $label$0
   (br_if $label$0
    (i32.lt_s
     (tee_local $6
      (i32.mul
       (get_local $3)
       (get_local $2)
      )
     )
     (i32.const 1)
    )
   )
   (set_local $9
    (i32.add
     (tee_local $8
      (i32.add
       (tee_local $3
        (i32.mul
         (get_local $9)
         (get_local $7)
        )
       )
       (get_local $6)
      )
     )
     (get_local $3)
    )
   )
   (set_local $11
    (f32.sub
     (f32.convert_s/i32
      (get_local $5)
     )
     (tee_local $10
      (f32.convert_s/i32
       (get_local $4)
      )
     )
    )
   )
   (set_local $3
    (i32.const 0)
   )
   (loop $label$1
    (block $label$2
     (br_if $label$2
      (i32.ge_s
       (tee_local $5
        (i32.add
         (tee_local $4
          (i32.add
           (i32.add
            (i32.mul
             (i32.div_s
              (tee_local $4
               (i32.div_s
                (get_local $3)
                (get_local $2)
               )
              )
              (i32.const 4)
             )
             (get_local $2)
            )
            (i32.div_s
             (i32.rem_s
              (get_local $3)
              (get_local $2)
             )
             (i32.const 2)
            )
           )
           (i32.and
            (i32.sub
             (i32.const 0)
             (i32.and
              (i32.div_s
               (get_local $4)
               (i32.const 2)
              )
              (i32.const 1)
             )
            )
            (get_local $7)
           )
          )
         )
         (get_local $6)
        )
       )
       (get_local $9)
      )
     )
     (br_if $label$2
      (i32.ge_s
       (tee_local $4
        (i32.add
         (get_local $4)
         (get_local $8)
        )
       )
       (get_local $9)
      )
     )
     (i32.store8
      (i32.add
       (get_local $1)
       (get_local $3)
      )
      (i32.trunc_u/f32
       (f32.mul
        (f32.sub
         (tee_local $12
          (select
           (f32.const 0)
           (f32.mul
            (f32.min
             (tee_local $12
              (f32.div
               (f32.sub
                (f32.convert_u/i32
                 (i32.load16_u
                  (get_local $0)
                 )
                )
                (get_local $10)
               )
               (get_local $11)
              )
             )
             (f32.const 1)
            )
            (f32.const 4)
           )
           (f32.lt
            (get_local $12)
            (f32.const 0)
           )
          )
         )
         (tee_local $12
          (f32.floor
           (get_local $12)
          )
         )
        )
        (f32.const 255)
       )
      )
     )
     (i32.store8
      (i32.add
       (get_local $1)
       (get_local $5)
      )
      (tee_local $5
       (i32.load8_u
        (i32.add
         (select
          (tee_local $5
           (i32.trunc_s/f32
            (get_local $12)
           )
          )
          (i32.const 4)
          (i32.lt_s
           (get_local $5)
           (i32.const 4)
          )
         )
         (i32.const 16)
        )
       )
      )
     )
     (i32.store8
      (i32.add
       (get_local $1)
       (get_local $4)
      )
      (get_local $5)
     )
    )
    (set_local $0
     (i32.add
      (get_local $0)
      (i32.const 2)
     )
    )
    (br_if $label$1
     (i32.ne
      (get_local $6)
      (tee_local $3
       (i32.add
        (get_local $3)
        (i32.const 1)
       )
      )
     )
    )
   )
  )
 )
)
