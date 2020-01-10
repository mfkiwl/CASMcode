#ifndef CASM_OccPerturbation
#define CASM_OccPerturbation

#include "casm/clusterography/ClusterInvariants.hh"
#include "casm/clusterography/GenericCluster.hh"
#include "casm/clusterography/CoordCluster.hh"
#include "casm/clex/HasCanonicalForm.hh"
#include "casm/kinetics/DoFTransformation.hh"
#include "casm/kinetics/OccupationTransformation.hh"
#include "casm/kinetics/OccupationTransformationTraits.hh"

namespace CASM {

  class Configuration;

  /// \brief Invariants of a OccPerturbation
  class OccPerturbationInvariants {

  public:

    OccPerturbationInvariants(const OccPerturbation &perturb);

    ClusterInvariants<IntegralCluster> cluster_invariants;
    std::map<std::string, Index> from_species_count;
    std::map<std::string, Index> to_species_count;
  };

  /// \brief Check if OccPerturbationInvariants are equal
  bool almost_equal(const OccPerturbationInvariants &A,
                    const OccPerturbationInvariants &B,
                    double tol);

  /// \brief Compare OccPerturbationInvariants
  bool compare(const OccPerturbationInvariants &A,
               const OccPerturbationInvariants &B,
               double tol);

  /// \brief Print OccPerturbationInvariants
  std::ostream &operator<<(std::ostream &sout, const OccPerturbationInvariants &obj);


  class OccPerturbation : public
    CanonicalForm <
    DoFTransformation <
    GenericCoordCluster <
    CRTPBase<OccPerturbation >>> > {

  public:

    typedef Structure PrimType;
    typedef traits<OccPerturbation>::Element Element;
    typedef traits<OccPerturbation>::InvariantsType InvariantsType;
    typedef traits<OccPerturbation>::size_type size_type;

    /// \brief Construct an empty UnitCellCoordCluster
    explicit OccPerturbation(const PrimType &_prim);

    /// \brief Construct a CoordCluster with a range of CoordType
    template<typename InputIterator>
    OccPerturbation(const PrimType &_prim,
                    InputIterator _begin,
                    InputIterator _end) :
      m_element(_begin, _end),
      m_prim_ptr(&_prim) {}

    /// \brief primtive structure of project
    const PrimType &prim() const;

    /// \brief Access vector of elements
    std::vector<Element> &elements();

    /// \brief const Access vector of elements
    const std::vector<Element> &elements() const;

    /// \brief cluster of sites this perturbation lives on
    const IntegralCluster &cluster() const;

    /// \brief inplace applies this perturbation to config
    Configuration &apply_to(Configuration &config) const;

    /// switches the initial and final states of this perturbation
    void reverse();

  protected:

    friend GenericCoordCluster<CRTPBase<OccPerturbation>>;
    friend DoFTransformation<GenericCoordCluster<CRTPBase<OccPerturbation>>>;

    /// \brief inplace applies the opposite of this perturbation
    Configuration &apply_reverse_to_impl(Configuration &config) const;

    /// \brief gives the ith coordinate of this perturbation cluster
    Coordinate coordinate_impl(size_type i) const;

  private:

    std::vector<OccupationTransformation> m_element;
    const PrimType *m_prim_ptr;

    // stores IntegralCluster, based on occ_transform uccoord
    mutable notstd::cloneable_ptr<IntegralCluster> m_cluster;

  };

  /// \brief Write OccPerturbation to JSON object
  jsonParser &to_json(const OccPerturbation &trans, jsonParser &json);

  template<>
  struct jsonConstructor<OccPerturbation> {

    static OccPerturbation from_json(const jsonParser &json, const Structure &prim);
  };

  /// \brief Print OccPerturbation to stream, using default Printer<OccPerturbation>
  std::ostream &operator<<(std::ostream &sout, const OccPerturbation &perturb);

  template<>
  struct Printer<OccPerturbation> : public PrinterBase {

    typedef OccPerturbation Element;
    static const std::string element_name;

    Printer(const OrbitPrinterOptions &_opt = OrbitPrinterOptions()) :
      PrinterBase(_opt) {}

    void print(const Element &element, Log &out);
  };
}

#endif
