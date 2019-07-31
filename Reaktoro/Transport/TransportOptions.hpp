// Reaktoro is a unified framework for modeling chemically systems.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library. If not, see <http://www.gnu.org/licenses/>.

#pragma once

namespace Reaktoro {

/// Options for finite volume methods.
enum class FiniteVolumeMethod
{
    FullImplicit,
    ImplicitExplicit,
    FluxLimitersImplicitExplicit
};

/// The options for the transport calculations.
struct TransportOptions
{
    /// The option for the finite volume scheme.
    FiniteVolumeMethod finite_volume_method = FiniteVolumeMethod::FullImplicit;
};

} // namespace Reaktoro