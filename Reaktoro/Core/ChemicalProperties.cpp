// Reaktoro is a unified framework for modeling chemically reactive systems.
//
// Copyright (C) 2014-2015 Allan Leal
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "ChemicalProperties.hpp"

// Reaktoro includes
#include <Reaktoro/Common/ChemicalScalar.hpp>
#include <Reaktoro/Common/Constants.hpp>
#include <Reaktoro/Common/Exception.hpp>
#include <Reaktoro/Common/NamingUtils.hpp>
#include <Reaktoro/Common/ReactionEquation.hpp>
#include <Reaktoro/Common/ThermoScalar.hpp>
#include <Reaktoro/Core/ChemicalSystem.hpp>
#include <Reaktoro/Core/Utils.hpp>
#include <Reaktoro/Thermodynamics/Models/PhaseChemicalModel.hpp>
#include <Reaktoro/Thermodynamics/Models/PhaseThermoModel.hpp>

namespace Reaktoro {

struct ChemicalProperties::Impl
{
    /// The chemical system
    ChemicalSystem system;

    /// The number of species in the system
    Index num_species = 0;

    /// The number of phases in the system
    Index num_phases = 0;

    /// The temperature of the system (in units of K)
    Temperature T;

    /// The pressure of the system (in units of Pa)
    Pressure P;

    /// The molar amounts of the species in the system (in units of mol).
    Vector n;

    /// The results of the evaluation of the PhaseThermoModel functions of each phase.
    ThermoModelResult tres;

    /// The results of the evaluation of the PhaseChemicalModel functions of each phase.
    ChemicalModelResult cres;

    /// Construct a default Impl instance
    Impl()
    {}

    /// Construct a Impl instance with given ChemicalSystem
    Impl(const ChemicalSystem& system)
    : system(system)
    {
        // Initialize the number of species and phases
        num_species = system.numSpecies();
        num_phases = system.numPhases();
    }

    /// Update the thermodynamic properties of the chemical system.
    auto update(double T_, double P_) -> void
    {
        // Set temperature and pressure
        T = T_;
        P = P_;

        // Calculate the thermodynamic properties of the system
        tres = system.thermoModel()(T_, P_);
    }

    /// Update the chemical properties of the chemical system.
    auto update(double T_, double P_, const Vector& n_) -> void
    {
        // Set temperature, pressure and composition
        T = T_;
        P = P_;
        n = n_;

        // Calculate the thermodynamic and chemical properties of the system
        tres = system.thermoModel()(T_, P_);
        cres = system.chemicalModel()(T_, P_, n_);
    }

    /// Return the molar fractions of the species.
    auto molarFractions() const -> ChemicalVector
    {
        ChemicalVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            const auto np = rows(n, offset, size);
            const auto xp = Reaktoro::molarFractions(np);
            res.rows(offset, offset, size, size) = xp;
            offset += size;
        }
        return res;
    }

    /// Return the ln activity coefficients of the species.
    auto lnActivityCoefficients() const -> ChemicalVector
    {
        ChemicalVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, offset, size, size) = cres[i].ln_activity_coefficients;
            offset += size;
        }
        return res;
    }

    /// Return the ln activity constants of the species.
    auto lnActivityConstants() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = cres[i].ln_activity_constants;
            offset += size;
        }
        return res;
    }

    /// Return the ln activities of the species.
    auto lnActivities() const -> ChemicalVector
    {
        ChemicalVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, offset, size, size) = cres[i].ln_activities;
            offset += size;
        }
        return res;
    }

    /// Return the chemical potentials of the species (in units of J/mol).
    auto chemicalPotentials() const -> ChemicalVector
    {
        const auto& R = universalGasConstant;
        const auto& G = standardPartialMolarGibbsEnergies();
        const auto& lna = lnActivities();
        return G + R*T*lna;
    }

    /// Return the standard partial molar Gibbs energies of the species (in units of J/mol).
    auto standardPartialMolarGibbsEnergies() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = tres[i].standard_partial_molar_gibbs_energies;
            offset += size;
        }
        return res;
    }

    /// Return the standard partial molar enthalpies of the species (in units of J/mol).
    auto standardPartialMolarEnthalpies() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = tres[i].standard_partial_molar_enthalpies;
            offset += size;
        }
        return res;
    }

    /// Return the standard partial molar volumes of the species (in units of m3/mol).
    auto standardPartialMolarVolumes() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = tres[i].standard_partial_molar_volumes;
            offset += size;
        }
        return res;
    }

    /// Return the standard partial molar entropies of the species (in units of J/(mol*K)).
    auto standardPartialMolarEntropies() const -> ThermoVector
    {
        const auto& G = standardPartialMolarGibbsEnergies();
        const auto& H = standardPartialMolarEnthalpies();
        return (H - G)/T;
    }

    /// Return the standard partial molar internal energies of the species (in units of J/mol).
    auto standardPartialMolarInternalEnergies() const -> ThermoVector
    {
        const auto& H = standardPartialMolarEnthalpies();
        const auto& V = standardPartialMolarVolumes();
        return H - P*V;
    }

    /// Return the standard partial molar Helmholtz energies of the species (in units of J/mol).
    auto standardPartialMolarHelmholtzEnergies() const -> ThermoVector
    {
        const auto& G = standardPartialMolarGibbsEnergies();
        const auto& V = standardPartialMolarVolumes();
        return G - P*V;
    }

    /// Return the standard partial molar isobaric heat capacities of the species (in units of J/(mol*K)).
    auto standardPartialMolarHeatCapacitiesConstP() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = tres[i].standard_partial_molar_heat_capacities_cp;
            offset += size;
        }
        return res;
    }

    /// Return the standard partial molar isochoric heat capacities of the species (in units of J/(mol*K)).
    auto standardPartialMolarHeatCapacitiesConstV() const -> ThermoVector
    {
        ThermoVector res(num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            res.rows(offset, size) = tres[i].standard_partial_molar_heat_capacities_cv;
            offset += size;
        }
        return res;
    }

    /// Return the molar Gibbs energies of the phases (in units of J/mol).
    auto phaseMolarGibbsEnergies() const -> ChemicalVector
    {
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            const auto np = rows(n, offset, size);
            const auto xp = Reaktoro::molarFractions(np);
            res.row(i, offset, size) = sum(xp % tres[i].standard_partial_molar_gibbs_energies);
            res.row(i, offset, size) += cres[i].residual_molar_gibbs_energy;
            offset += size;
        }
        return res;
    }

    /// Return the molar enthalpies of the phases (in units of J/mol).
    auto phaseMolarEnthalpies() const -> ChemicalVector
    {
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            const auto np = rows(n, offset, size);
            const auto xp = Reaktoro::molarFractions(np);
            res.row(i, offset, size) = sum(xp % tres[i].standard_partial_molar_enthalpies);
            res.row(i, offset, size) += cres[i].residual_molar_enthalpy;
            offset += size;
        }
        return res;
    }

    /// Return the molar volumes of the phases (in units of m3/mol).
    auto phaseMolarVolumes() const -> ChemicalVector
    {
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            if(cres[i].molar_volume.val > 0.0)
                res.row(i, offset, size) = cres[i].molar_volume;
            else
            {
                const auto np = rows(n, offset, size);
                const auto xp = Reaktoro::molarFractions(np);
                res.row(i, offset, size) = sum(xp % tres[i].standard_partial_molar_volumes);
            }

            offset += size;
        }
        return res;
    }

    /// Return the molar entropies of the phases (in units of J/(mol*K)).
    auto phaseMolarEntropies() const -> ChemicalVector
    {
        const auto& G = phaseMolarGibbsEnergies();
        const auto& H = phaseMolarEnthalpies();
        return (H - G)/T;
    }

    /// Return the molar internal energies of the phases (in units of J/mol).
    auto phaseMolarInternalEnergies() const -> ChemicalVector
    {
        const auto& H = phaseMolarEnthalpies();
        const auto& V = phaseMolarVolumes();
        return H - P*V;
    }

    /// Return the molar Helmholtz energies of the phases (in units of J/mol).
    auto phaseMolarHelmholtzEnergies() const -> ChemicalVector
    {
        const auto& G = phaseMolarGibbsEnergies();
        const auto& V = phaseMolarVolumes();
        return G - P*V;
    }

    /// Return the molar isobaric heat capacities of the phases (in units of J/(mol*K)).
    auto phaseMolarHeatCapacitiesConstP() const -> ChemicalVector
    {
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            const auto np = rows(n, offset, size);
            const auto xp = Reaktoro::molarFractions(np);
            res.row(i, offset, size) = sum(xp % tres[i].standard_partial_molar_heat_capacities_cp);
            res.row(i, offset, size) += cres[i].residual_molar_heat_capacity_cp;
            offset += size;
        }
        return res;
    }

    /// Return the molar isochoric heat capacities of the phases (in units of J/(mol*K)).
    auto phaseMolarHeatCapacitiesConstV() const -> ChemicalVector
    {
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            const auto np = rows(n, offset, size);
            const auto xp = Reaktoro::molarFractions(np);
            res.row(i, offset, size) = sum(xp % tres[i].standard_partial_molar_heat_capacities_cv);
            res.row(i, offset, size) += cres[i].residual_molar_heat_capacity_cv;
            offset += size;
        }
        return res;
    }

    /// Return the specific Gibbs energies of the phases (in units of J/kg).
    auto phaseSpecificGibbsEnergies() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarGibbsEnergies();
    }

    /// Return the specific enthalpies of the phases (in units of J/kg).
    auto phaseSpecificEnthalpies() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarEnthalpies();
    }

    /// Return the specific volumes of the phases (in units of m3/kg).
    auto phaseSpecificVolumes() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarVolumes();
    }

    /// Return the specific entropies of the phases (in units of J/(kg*K)).
    auto phaseSpecificEntropies() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarEntropies();
    }

    /// Return the specific internal energies of the phases (in units of J/kg).
    auto phaseSpecificInternalEnergies() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarInternalEnergies();
    }

    /// Return the specific Helmholtz energies of the phases (in units of J/kg).
    auto phaseSpecificHelmholtzEnergies() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarHelmholtzEnergies();
    }

    /// Return the specific isobaric heat capacities of the phases (in units of J/(kg*K)).
    auto phaseSpecificHeatCapacitiesConstP() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarHeatCapacitiesConstP();
    }

    /// Return the specific isochoric heat capacities of the phases (in units of J/(kg*K)).
    auto phaseSpecificHeatCapacitiesConstV() const -> ChemicalVector
    {
        return phaseAmounts()/phaseMasses() % phaseMolarHeatCapacitiesConstV();
    }

    /// Return the densities of the phases (in units of kg/m3).
    auto phaseDensities() const -> ChemicalVector
    {
        return phaseMasses()/(phaseAmounts() % phaseMolarVolumes());
    }

    /// Return the masses of the phases (in units of kg).
    auto phaseMasses() const -> ChemicalVector
    {
        auto nc = Reaktoro::composition(n);
        auto mm = Reaktoro::molarMasses(system.species());
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            auto np = nc.rows(offset, offset, size, size);
            auto mmp = rows(mm, offset, size);
            res.row(i, offset, size) = sum(mmp % np);
            offset += size;
        }
        return res;
    }

    /// Return the molar amounts of the phases (in units of mol).
    auto phaseAmounts() const -> ChemicalVector
    {
        auto nc = Reaktoro::composition(n);
        ChemicalVector res(num_phases, num_species);
        unsigned offset = 0;
        for(unsigned i = 0; i < num_phases; ++i)
        {
            const unsigned size = system.numSpeciesInPhase(i);
            auto np = nc.rows(offset, offset, size, size);
            res.row(i, offset, size) = sum(np);
            offset += size;
        }
        return res;
    }

    /// Return the volumes of the phases (in units of m3).
    auto phaseVolumes() const -> ChemicalVector
    {
        return phaseAmounts() % phaseMolarVolumes();
    }

    /// Return the volume of the system (in units of m3).
    auto volume() const -> ChemicalScalar
    {
        return sum(phaseAmounts() % phaseMolarVolumes());
    }

    /// Return the pH of the system.
    auto pH() const -> ChemicalScalar
    {
        // Find index of aqueous phase
        const Index iwater = system.indexSpeciesAny(alternativeWaterNames());
        const Index iaqueous = system.indexPhaseWithSpecies(iwater);

        // Check there is an aqueous phase in the system
        if(iaqueous >= system.numPhases())
            return ChemicalScalar(system.numSpecies());

        // Find the local index of H+ in the aqueous phase
        const Index ihydron = system.phase(iaqueous).
            indexSpeciesAny(alternativeChargedSpeciesNames("H+"));

        // The number of aqueous species and its first species index
        const Index size = system.numSpeciesInPhase(iaqueous);
        const Index ifirst = system.indexFirstSpeciesInPhase(iaqueous);

        // Check there is a hydron species in the aqueous phase
        if(ihydron >= size)
            return ChemicalScalar(system.numSpecies());

        // Calculate pH
        ChemicalScalar pH = -cres[iaqueous].ln_activities[ihydron]/std::log(10);

        // Resize the derivative vector from number of aqueous species to total number of species
        pH.ddn.conservativeResize(system.numSpecies());
        rows(pH.ddn, ifirst, size) = rows(pH.ddn, 0, size);

        return pH;
    }

    /// Return the pe of the system.
    auto pe() const -> ChemicalScalar
    {
        return pe(ReactionEquation("H2(aq) = 2*H+ + 2*e-"));
    }

    /// Return the pe of the system.
    auto pe(const ReactionEquation& reaction) const -> ChemicalScalar
    {
        // The RT constant
        const ThermoScalar RT = universalGasConstant * Temperature(T);

        // Find index of aqueous phase
        const Index iwater = system.indexSpeciesAny(alternativeWaterNames());
        const Index iaqueous = system.indexPhaseWithSpecies(iwater);

        // Check there is an aqueous phase in the system
        if(iaqueous >= system.numPhases())
            return ChemicalScalar(system.numSpecies());

        // Find the stoichiometry of e-
        double stoichiometry_eminus = 0.0;
        if(stoichiometry_eminus == 0.0) stoichiometry_eminus = reaction.stoichiometry("e-");
        if(stoichiometry_eminus == 0.0) stoichiometry_eminus = reaction.stoichiometry("e[-]");

        // Assert the stoichiometry of e- is positive
        Assert(stoichiometry_eminus != 0.0, "Could not calculate the pe of the system.",
            "There is no `e-` or `e[-]` species in the half reaction.");

        // Find the index of e- in the aqueous phase
        const Index ieminus = system.phase(iaqueous).indexSpeciesAny(
            alternativeChargedSpeciesNames("e-"));

        // The number of aqueous species and its first species index
        const Index size = system.numSpeciesInPhase(iaqueous);
        const Index ifirst = system.indexFirstSpeciesInPhase(iaqueous);

        // Find the standard chemical potential of e- (it is not zero if the
        // standard chemical potentials were obtained from log(k)'s of reactions.
        ThermoScalar G0_eminus;
        if(ieminus < size)
            G0_eminus = tres[iaqueous].standard_partial_molar_gibbs_energies[ieminus];

        // The pe of the system
        ChemicalScalar pe(size);

        // Loop over all species in the reaction
        for(auto pair : reaction.equation())
        {
            // Skip if current species is either e- or e[-]
            if(pair.first == "e-" || pair.first == "e[-]")
                continue;

            // Find the local index of the current species and its phase index
            const Index ispecies = system.phase(iaqueous).indexSpeciesWithError(pair.first);

            // Get the standard chemical potential of the current species
            const ThermoScalar G0i = tres[iaqueous].
                standard_partial_molar_gibbs_energies[ispecies]/RT;

            // Get the ln activity of the current species
            const ChemicalScalar ln_ai = cres[iaqueous].
                ln_activities[ispecies];

            // Get the stoichiometry of current species
            const double stoichiometry = pair.second;

            // Update contribution
            pe -= stoichiometry * (G0i + ln_ai);
        }

        // Finalize the calculation of pe
        pe /= stoichiometry_eminus;
        pe -= G0_eminus;
        pe /= -std::log(10);

        // Resize the derivative vector from number of aqueous species to total number of species
        pe.ddn.conservativeResize(system.numSpecies());
        rows(pe.ddn, ifirst, size) = rows(pe.ddn, 0, size);

        return pe;
    }
};

ChemicalProperties::ChemicalProperties()
: pimpl(new Impl())
{}

ChemicalProperties::ChemicalProperties(const ChemicalSystem& system)
: pimpl(new Impl(system))
{}

ChemicalProperties::ChemicalProperties(const ChemicalProperties& other)
: pimpl(new Impl(*other.pimpl))
{}

ChemicalProperties::~ChemicalProperties()
{}

auto ChemicalProperties::operator=(ChemicalProperties other) -> ChemicalProperties&
{
    pimpl = std::move(other.pimpl);
    return *this;
}

auto ChemicalProperties::update(double T, double P) -> void
{
    pimpl->update(T, P);
}

auto ChemicalProperties::update(double T, double P, const Vector& n) -> void
{
    pimpl->update(T, P, n);
}

auto ChemicalProperties::temperature() const -> double
{
    return pimpl->T.val;
}

auto ChemicalProperties::pressure() const -> double
{
    return pimpl->P.val;
}

auto ChemicalProperties::composition() const -> const Vector&
{
    return pimpl->n;
}

auto ChemicalProperties::molarFractions() const -> ChemicalVector
{
    return pimpl->molarFractions();
}

auto ChemicalProperties::lnActivityCoefficients() const -> ChemicalVector
{
    return pimpl->lnActivityCoefficients();
}

auto ChemicalProperties::lnActivityConstants() const -> ThermoVector
{
    return pimpl->lnActivityConstants();
}

auto ChemicalProperties::lnActivities() const -> ChemicalVector
{
    return pimpl->lnActivities();
}

auto ChemicalProperties::chemicalPotentials() const -> ChemicalVector
{
    return pimpl->chemicalPotentials();
}

auto ChemicalProperties::standardPartialMolarGibbsEnergies() const -> ThermoVector
{
    return pimpl->standardPartialMolarGibbsEnergies();
}

auto ChemicalProperties::standardPartialMolarEnthalpies() const -> ThermoVector
{
    return pimpl->standardPartialMolarEnthalpies();
}

auto ChemicalProperties::standardPartialMolarVolumes() const -> ThermoVector
{
    return pimpl->standardPartialMolarVolumes();
}

auto ChemicalProperties::standardPartialMolarEntropies() const -> ThermoVector
{
    return pimpl->standardPartialMolarEntropies();
}

auto ChemicalProperties::standardPartialMolarInternalEnergies() const -> ThermoVector
{
    return pimpl->standardPartialMolarInternalEnergies();
}

auto ChemicalProperties::standardPartialMolarHelmholtzEnergies() const -> ThermoVector
{
    return pimpl->standardPartialMolarHelmholtzEnergies();
}

auto ChemicalProperties::standardPartialMolarHeatCapacitiesConstP() const -> ThermoVector
{
    return pimpl->standardPartialMolarHeatCapacitiesConstP();
}

auto ChemicalProperties::standardPartialMolarHeatCapacitiesConstV() const -> ThermoVector
{
    return pimpl->standardPartialMolarHeatCapacitiesConstV();
}

auto ChemicalProperties::phaseMolarGibbsEnergies() const -> ChemicalVector
{
    return pimpl->phaseMolarGibbsEnergies();
}

auto ChemicalProperties::phaseMolarEnthalpies() const -> ChemicalVector
{
    return pimpl->phaseMolarEnthalpies();
}

auto ChemicalProperties::phaseMolarVolumes() const -> ChemicalVector
{
    return pimpl->phaseMolarVolumes();
}

auto ChemicalProperties::phaseMolarEntropies() const -> ChemicalVector
{
    return pimpl->phaseMolarEntropies();
}

auto ChemicalProperties::phaseMolarInternalEnergies() const -> ChemicalVector
{
    return pimpl->phaseMolarInternalEnergies();
}

auto ChemicalProperties::phaseMolarHelmholtzEnergies() const -> ChemicalVector
{
    return pimpl->phaseMolarHelmholtzEnergies();
}

auto ChemicalProperties::phaseMolarHeatCapacitiesConstP() const -> ChemicalVector
{
    return pimpl->phaseMolarHeatCapacitiesConstP();
}

auto ChemicalProperties::phaseMolarHeatCapacitiesConstV() const -> ChemicalVector
{
    return pimpl->phaseMolarHeatCapacitiesConstV();
}

auto ChemicalProperties::phaseSpecificGibbsEnergies() const -> ChemicalVector
{
    return pimpl->phaseSpecificGibbsEnergies();
}

auto ChemicalProperties::phaseSpecificEnthalpies() const -> ChemicalVector
{
    return pimpl->phaseSpecificEnthalpies();
}

auto ChemicalProperties::phaseSpecificVolumes() const -> ChemicalVector
{
    return pimpl->phaseSpecificVolumes();
}

auto ChemicalProperties::phaseSpecificEntropies() const -> ChemicalVector
{
    return pimpl->phaseSpecificEntropies();
}

auto ChemicalProperties::phaseSpecificInternalEnergies() const -> ChemicalVector
{
    return pimpl->phaseSpecificInternalEnergies();
}

auto ChemicalProperties::phaseSpecificHelmholtzEnergies() const -> ChemicalVector
{
    return pimpl->phaseSpecificHelmholtzEnergies();
}

auto ChemicalProperties::phaseSpecificHeatCapacitiesConstP() const -> ChemicalVector
{
    return pimpl->phaseSpecificHeatCapacitiesConstP();
}

auto ChemicalProperties::phaseSpecificHeatCapacitiesConstV() const -> ChemicalVector
{
    return pimpl->phaseSpecificHeatCapacitiesConstV();
}

auto ChemicalProperties::phaseDensities() const -> ChemicalVector
{
    return pimpl->phaseDensities();
}

auto ChemicalProperties::phaseMasses() const -> ChemicalVector
{
    return pimpl->phaseMasses();
}

auto ChemicalProperties::phaseAmounts() const -> ChemicalVector
{
    return pimpl->phaseAmounts();
}

auto ChemicalProperties::phaseVolumes() const -> ChemicalVector
{
    return pimpl->phaseVolumes();
}

auto ChemicalProperties::volume() const -> ChemicalScalar
{
    return pimpl->volume();
}

auto ChemicalProperties::pH() const -> ChemicalScalar
{
    return pimpl->pH();
}

auto ChemicalProperties::pe() const -> ChemicalScalar
{
    return pimpl->pe();
}

auto ChemicalProperties::pe(std::string reaction) const -> ChemicalScalar
{
    return pimpl->pe(reaction);
}

} // namespace Reaktoro
