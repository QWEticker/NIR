# Verification and validation protocol for attack articles

## Scope

The simulator is verified against the declared optical data path and validated
against calculations that do not import or link the C++ attack, estimator, or
security classes. Verification establishes implementation conformance; it does
not establish finite-key or composable security.

The optical convention is the coherent amplitude \(a\), with
\(q=2\operatorname{Re}(a)\), \(p=2\operatorname{Im}(a)\), and vacuum variance
equal to one SNU. The QPSK modulation variance is \(V_A=2\alpha^2\).

## Intercept-resend scenarios

Eve intercepts a pulse with probability \(f\), uses resend gain \(g\), and has
heterodyne parameters \(\eta_E\) and \(v_{\mathrm{el},E}\). The ideal Eve is a
physical unit-efficiency, zero-electronic-noise heterodyne receiver. It still
adds two SNU in a full measure-and-prepare attack. The real Eve copies Bob's
receiver parameters. The noiseless oracle is retained only as a numerical
control and is not a physical Eve model.

Without clipping, define

\[
G=1+f(g-1),
\qquad
T_{\mathrm{eff}}=T G^2,
\]

\[
\xi_{\mathrm{eff}}=
\frac{\xi+
2fg^2(1+v_{\mathrm{el},E})/\eta_E+
V_A f(1-f)(g-1)^2}{G^2}.
\]

For the oracle, the measurement-noise term is zero. If \(G=0\), transmission is
zero and input-referred excess noise is not identifiable. Partial interception
is a mixture rather than a Gaussian residual model, so Gaussian interval
coverage is not claimed for \(0<f<1\).

The fixed anchor \(T=0.6\), \(\xi=0.01\), \(\eta_E=0.6\),
\(v_{\mathrm{el},E}=0.05\), \(f=1\), \(g=0.8\) gives
\(T_{\mathrm{eff}}=0.384\) and \(\xi_{\mathrm{eff}}=3.515625\).

## Collective channel scenarios

The implemented collective attack is a Bob-side Gaussian channel-statistics
model. It does not simulate Eve's quantum memory or a joint collective
measurement. For amplitude coupling \(c\),

\[
t=1-c^2,
\qquad
T_{\mathrm{eff}}=Tt,
\qquad
\xi_{\mathrm{eff}}=\xi+\frac{\xi_{\mathrm{add}}}{T}.
\]

The last expression is defined only for \(T>0\). Pure loss
\((\xi_{\mathrm{add}}=0)\) changes transmission without adding excess noise.
The fixed anchor \(T=0.6\), \(\xi=0.01\), \(c=0.3\),
\(\xi_{\mathrm{add}}=0.05\) gives \(T_{\mathrm{eff}}=0.546\) and
\(\xi_{\mathrm{eff}}=0.09333333333333334\).

## Statistical campaign

The publication campaign in `validation/article_cases.json` fixes:

- \(N=100000\) pulses per run;
- 200 independent runs per selected table row;
- a fixed base seed;
- raw run-level estimates and a summary containing mean, bias, standard
  deviation, standard error, RMSE, and interval coverage;
- exact Clopper-Pearson bounds for empirical coverage.

For Gaussian regimes, a row passes when the absolute mean bias is no larger
than the greater of the declared numerical floor and four standard errors, and
the 95% Clopper-Pearson interval contains the nominal 95% joint coverage.
Coverage is deliberately omitted for partial IR, clipping, zero effective
transmission, and other misspecified regimes.

Run the independent oracle without the simulator:

```text
python scripts/independent_attack_oracle.py --cases validation/article_cases.json --out-dir results/article_validation/oracle --publication-only
```

Use `--analytical-only` for the deterministic reference table, or override
`--samples` and `--repeats` for a quick non-publication smoke run. The output
manifest records input and output SHA-256 hashes, seed, dimensions, Python
version, and platform.

Run the paired simulator/oracle campaign after building `cvqkd_sim`:

```text
python scripts/run_article_validation.py --exe build/cvqkd_sim --cases validation/article_cases.json --out-dir results/article_validation --publication-only
```

The paired run preserves each generated simulator configuration, raw outputs,
separate simulator and oracle summaries, a comparison table, and a top-level
manifest tied to the executable hash and embedded source revision.

## Article-safe conclusions

- Agreement of \(T\), \(\xi\), Alice-Bob statistics, and Alice-Eve statistics
  with the declared scenario supports verification and numerical validation.
- The ideal, real, and oracle Eve results must be reported separately.
- Gaussian Holevo calculations are comparison references under their stated
  assumptions, not proofs for every QPSK or nonlinear scenario.
- A positive asymptotic key-rate estimate is not a finite-key certificate.
- Collective-attack validation applies to Bob-side channel statistics unless a
  full Eve dilation and measurement are explicitly included.
