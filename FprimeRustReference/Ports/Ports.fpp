module FprimeRustReference {

  @ Carries a counter value between the Rust and C++ reference components
  port Count(
    count: U32 @< Counter value being reported
  )

}
