# Initialization
def getNumberOfUses(v):


# Initialize empty set of active intervals
active_intervals = {}

for each variable v in order of increasing start point of their live intervals:
    if len(active_intervals) < num_registers:
        # Allocate a register to v
        active_intervals.add(v)
        continue

    # Find the variable to spill
    spill_var = select_spill_var(active_intervals)

    if spill_var.live_interval.start < v.live_interval.start:
        # Spill the variable and allocate a register to v
        spill(spill_var)
        active_intervals.remove(spill_var)
        active_intervals.add(v)
    else:
        # Spill v
        spill(v)

def select_spill_var(active_intervals):
    # Pick the variable with the furthest next use and the lowest spill cost
    furthest_next_use = -1
    lowest_spill_cost = float('inf')
    spill_var = None

    for v in active_intervals:
        if v.next_use > furthest_next_use or (v.next_use == furthest_next_use and v.spill_cost > lowest_spill_cost):
            furthest_next_use = v.next_use
            lowest_spill_cost = v.spill_cost
            spill_var = v

    return spill_var