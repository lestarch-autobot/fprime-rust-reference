module FprimeRustReference {

  @ Passive C++ component that forwards counts from a Rust component to a Rust component
  passive component Relay {

    @ Count from the Rust producer
    sync input port countIn: FprimeRustReference.Count

    @ Count forwarded to the Rust consumer
    output port countOut: FprimeRustReference.Count

    @ Time stamping for telemetry
    time get port timeCaller

    @ Standard telemetry interface
    import Fw.Channel

    @ Number of counts relayed
    telemetry Relayed: U32 id 0

  }

}
