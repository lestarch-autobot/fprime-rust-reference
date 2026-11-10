// ======================================================================
// \title  Relay.cpp
// \brief  Passive C++ component forwarding counts between the Rust components
// ======================================================================
#include "FprimeRustReference/Components/Relay/Relay.hpp"

namespace FprimeRustReference {

Relay::Relay(const char* const compName) : RelayComponentBase(compName), m_relayed(0) {}

Relay::~Relay() {}

void Relay::countIn_handler(FwIndexType portNum, U32 count) {
    if (this->isConnected_countOut_OutputPort(0)) {
        this->countOut_out(0, count);
    }
    this->m_relayed++;
    this->tlmWrite_Relayed(this->m_relayed);
}

}  // namespace FprimeRustReference
