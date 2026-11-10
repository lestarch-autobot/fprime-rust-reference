// ======================================================================
// \title  Relay.hpp
// \brief  Passive C++ component forwarding counts between the Rust components
// ======================================================================
#ifndef FprimeRustReference_Relay_HPP
#define FprimeRustReference_Relay_HPP

#include "FprimeRustReference/Components/Relay/RelayComponentAc.hpp"

namespace FprimeRustReference {

class Relay final : public RelayComponentBase {
  public:
    explicit Relay(const char* const compName);
    ~Relay();

  private:
    //! Forward the count and report the running total (REQ-Relay-001, -002)
    void countIn_handler(FwIndexType portNum, U32 count) override;

    U32 m_relayed;  //!< Number of counts relayed
};

}  // namespace FprimeRustReference

#endif
