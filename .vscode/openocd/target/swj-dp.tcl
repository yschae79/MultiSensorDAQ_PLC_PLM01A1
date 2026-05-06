# ARM Debug Interface V5 (ADI_V5) utility - PATCHED
#
# Patch for ST OpenOCD 0.12.0+dev where 'hla newtap' was deprecated
# and internally redirected to 'swj_newdap', causing infinite recursion.
# Fix: use 'swd newdap' for both hla and swd transports.

if [catch {transport select}] {
  echo "Error: unable to select a session transport. Can't continue."
  shutdown
}

proc swj_newdap {chip tag args} {
    if [using_jtag] {
        eval jtag newtap $chip $tag $args
    } else {
        # For both hla_swd and swd transports: use swd newdap
        # (ST OpenOCD 0.12.0 deprecated 'hla newtap' -> use swd newdap)
        eval swd newdap $chip $tag $args
    }
}
