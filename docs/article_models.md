# Models and validation contract

## Units and trust boundary

The optical samples are coherent-state amplitudes `a`, not measured quadratures.
With `[q,p]=2i`, `q=2 Re(a)`, `p=2 Im(a)`, the vacuum variance is 1 SNU.
Uniform QPSK has mean photon number `alpha²` and quadrature modulation variance
`V_A=2 alpha²`. The legacy `modulation_variance()` accessor returns photon number;
the security calculation uses `quadrature_variance()`.

A channel maps `a -> sqrt(T) a + z`, with independent real components of `z`
having variance `T xi / 4`. `xi` is input-referred excess *variance*, in SNU.
A trusted heterodyne receiver outputs `sqrt(eta) Re(a)` and
`sqrt(eta) Im(a)` plus independent noise of variance `(1+v_el)/2`.
Thus the regression residual variance per component is
`eta T xi / 4 + (1+v_el)/2`. Shot noise must not be added again by a loss-only
optical channel. It is included when a coherent state is measured.

IR acts at the source output, before the transmission channel.
The collective Gaussian channel acts after transmission, before Bob's detector.
LO and saturation transformations act on detector outputs. Models containing
these nonlinear or calibration attacks must not inherit a security claim from
the trusted linear Gaussian model.

## Intercept and resend

Eve intercepts each pulse independently with probability `fraction`.
She heterodynes the intercepted coherent state, divides the outcome by
`sqrt(eta_E)` for calibration, and prepares a new coherent state with amplitude
`resend_gain` times that calibrated outcome.

* `ideal`: quantum-limited heterodyne, `eta_E=1`, `v_el_E=0`, no saturation.
  Each calibrated component still has variance 1/2. Full IR at unit gain
  therefore adds **2 input-referred SNU**, not zero.
* `real`: the same detector law as Bob, including efficiency, electronic noise
  and saturation. Defaults are copied from Bob; overrides are recorded.
* `oracle`: noiseless access to the original amplitude, explicitly an
  unphysical numerical control. It cannot support a physical security claim.

The attack stores its outcomes and interception mask. Missing outcomes are
erasures, not invented measurements. Alice–Bob and Alice–Eve frequencies use
the same nearest-phase QPSK decisions and Gray-coded bit labels. Frequencies,
mutual information and error rates describe classical measurements; Eve's
measured mutual information must not replace a collective Holevo bound.

For full physical IR with unit resend gain, no clipping and transmission after
Eve, the excess-noise increment is `2 (1+v_el_E)/eta_E`. The resulting
measure-and-prepare channel is entanglement breaking. Partial IR and nonunit
gain are mixtures; a Gaussian likelihood is not assumed for their intervals.

## Collective Gaussian channel

`coupling` is an amplitude beam-splitter coefficient in `[0,1]`;
the attack's power transmission is `t=1-coupling²`. `excess_noise` is
input-referred excess variance of this additional channel, **at its own input**.
Its optical displacement variance per component is `t excess_noise/4`.
Consequently, after a preceding channel `(T,xi)`,

* total transmission is `T t`;
* total input-referred excess noise is `xi + excess_noise/T` when `T t>0`;
* vacuum coupling alone changes transmission and adds no excess noise;
* `t=1` with nonzero excess is the additive-noise limit;
* at zero transmission, input-referred noise cannot be identified.

The thermal environment of a beam-splitter dilation has variance
`W=1+t excess_noise/(1-t)`. A complete entangling-cloner covariance description
contains Bob and **two** Eve modes, including the purification of the thermal
environment. Classical displacement sampling reproduces Bob's statistics;
it does not simulate a quantum memory or a joint quantum measurement.
The analytical channel predictions are comparison oracles only and are never
added to already measured excess noise.

## Evidence and scope

Validation must compare independent calculations, not two calls to the same
implementation: receiver/channel moments; IR's 2-SNU limit; symplectic spectra
and Gaussian Holevo values from an explicit covariance dilation; QPSK
statistics; parameter-interval coverage; and complete repeated experiments.
Seeds, actual run count, configuration, raw estimates, confidence assumptions
and code revision belong in the outputs.

A positive asymptotic expression is not a generated or certified finite key.
Gaussian-modulation reference rates must be labelled separately from QPSK.
Denys–Brown–Leverrier's correlation bound concerns asymptotic collective-attack
analysis with its stated source and channel-statistics assumptions. Finite-key
composable security, authenticated reconciliation/privacy amplification,
hardware calibration and agreement with the author's Mathcad file require
separate evidence.

## Sources

1. Laudenbach et al. (2018), DOI: 10.1002/qute.201800011.
2. Laudenbach and Pacher (2019), DOI: 10.1002/qute.201900055.
3. Lodewyck et al. (2007), DOI: 10.1103/PhysRevLett.98.030503.
   The full IR reference is 2 SNU; their homodyne/Gaussian experiment is not
   numerically interchangeable with QPSK/heterodyne detection.
4. Denys, Brown, Leverrier (2021), DOI: 10.22331/q-2021-09-13-540,
   https://arxiv.org/abs/2103.13945.
