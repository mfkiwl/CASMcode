#ifndef CASM_DiffTransConfigInterpolation
#define CASM_DiffTransConfigInterpolation

#include "casm/container/Counter.hh"
#include "casm/container/RandomAccessEnumerator.hh"
#include "casm/clex/Configuration.hh"
#include "casm/misc/cloneable_ptr.hh"
#include "casm/clex/ConfigEnumInterpolation.hh"
#include "casm/kinetics/DiffTransConfiguration.hh"
#include "casm/kinetics/DiffusionTransformation.hh"
// extern "C" {
//   CASM::EnumInterfaceBase *make_DiffTransConfigInterpolation_interface();
// }

namespace CASM {

  namespace Kinetics {
    /** \defgroup ConfigEnumGroup Configuration Enumerators
     *  \ingroup Configuration
     *  \ingroup Enumerator
     *  \brief Enumerates Configuration
     *  @{
     */

    /// \brief Enumerate over all possible occupations in a particular Supercell
    ///
    class DiffTransConfigInterpolation : public RandomAccessEnumeratorBase<Configuration> {

      // -- Required members -------------------

    public:

      /// \brief Construct with a Supercell, using all permutations
      DiffTransConfigInterpolation(const DiffTransConfiguration &_diff_trans_config, const int n_images);

      std::string name() const override {
        return enumerator_name;
      }

      static const std::string enumerator_name;
      static const std::string interface_help;

      static int run(const PrimClex &primclex, const jsonParser &kwargs, const Completer::EnumOption &enum_opt);

    private:
      // Returns to config that are edited to have same occupants on the diff_trans_orbit
      // output used to intepolate rest of the configuration other that diff trans orbit

      Configuration m_current;
      DiffTransConfiguration m_diff_trans_config;
      std::unique_ptr<ConfigEnumInterpolation> m_config_enum_interpol;

      /// Implements at_step
      const Configuration *at_step(step_type n) override;

      // // -- Unique -------------------
      Configuration prepare_to_config(const Configuration &config, const DiffusionTransformation &diff_trans);
    };
    Configuration get_relaxed_config(const DiffTransConfiguration &dfc, Configuration config);
  }
}
#endif
