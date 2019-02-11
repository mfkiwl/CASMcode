#ifndef CASM_MagSpinDoFTraits
#define CASM_MagSpinDoFTraits
#include "casm/basis_set/DoFTraits.hh"

#include "casm/basis_set/BasisSet.hh"
#include "casm/crystallography/Site.hh"

namespace CASM {
  namespace DoF_impl {
    class MagSpinDoFTraits : public DoFType::Traits {
    public:
      MagSpinDoFTraits():
        DoFType::Traits("magspin", {
        "sx", "sy", "sz"
      },
      DoFType::CONTINUOUS,
      DoFType::LOCAL,
      true) {
      }

      bool time_reversal_active() const override {
        //std::cout << "IS TIME-REVERSAL ACTIVE\n";
        return true;
      }

      /// \brief Output @param _in to JSON
      void to_json(DoFSet const &_out, jsonParser &_json) const override {
        throw std::runtime_error("MagSpinDoFTraits::to_json not implemented!");
      }

      Eigen::MatrixXd symop_to_matrix(SymOp const &op) const override;

      /// \brief Construct the site basis (if DOF_MODE is LOCAL) for a DoF, given its site
      std::vector<BasisSet> construct_site_bases(Structure const &_prim,
                                                 std::vector<Orbit<IntegralCluster, PrimPeriodicSymCompare<IntegralCluster> > > &_asym_unit,
                                                 jsonParser const &_bspecs) const override;
    protected:
      DoFType::BasicTraits *_clone() const override {
        return new MagSpinDoFTraits(*this);
      }
    };
  }

  namespace DoFType {
    DoF_impl::MagSpinDoFTraits magspin();
  }

}
#endif
