# Functions in this script are used to generate low level gcode
# instructions for the primary knits used by the machine.

import math
import json

HOME_POSITIONS_FP = "../home_positions.json"
TOOL_POSITIONS_FP = "../tool_positions.json"

with open(HOME_POSITIONS_FP, "r") as infile:
    home_positions = json.load(infile)

with open(TOOL_POSITIONS_FP, "r") as infile:
    tool_positions = json.load(infile)


def generate_tool_move(i, home_x, home_y, tool):
    tool_x = tool_positions[tool][0] + home_x
    tool_y = tool_positions[tool][1] + home_y
    return "G1 X{} Y{}".format(tool_x, tool_y)


def generate_move(x, y):
    '''Helper method to easily generate a string with proper GCode syntax/structure'''
    return "G1 X{} Y{}".format(x, y)


def generate_circle_path(i, home_x, home_y, radius, theta_i, num_samples):
    '''Helper method to easily generate gcode to move along the path of a circle 
    by sampling points along the circumference for a given radial angle
        x and y: coordinates for the center of the circle
        radius: radius of the circle
        theta_i: angle for which the path will be traversed
        num_samples: number of samples along the circumference to use, lower -> coarser, higher -> finer '''
    instructions = []
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E-1")
    instructions.append("G1 Z17")
    increment = 1
    x = 0
    y = 0
    for i in range(num_samples):
        x = home_x - radius * math.sin(math.radians(theta_i - increment * i))
        y = home_y + radius * math.cos(math.radians(theta_i - increment * i))
        instructions.append(generate_move(x, y))
    instructions.append(generate_move(x - 25, y))
    return instructions


def generate_cast_on(i, home_x, home_y, radius, theta_i, num_samples):
    '''Generates gcode to cast a loop onto a needle'''
    instructions = []
    instructions.append(generate_move(home_x - 10, home_y - 10))
    instructions.append("G92 E0")
    instructions.append("G1 E3")
    instructions += generate_circle_path(i, home_x, home_y, radius, theta_i, num_samples)
    instructions.append("G92 E0")
    instructions.append("G1 E3")
    instructions += backwards_brush(i, home_x, home_y)
    instructions += pinch_grab(i, home_x, home_y)
    instructions += forward_brush(i, home_x, home_y)
    return instructions


def pinch_grab(i, home_x, home_y):
    '''Generates gcode to grab yarn from the pinch arm'''
    instructions = []
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E-1")
    instructions.append("G1 Z0")
    instructions.append(generate_move(home_x - 77, home_y + 27))
    instructions.append(generate_move(home_x - 35, home_y + 27))
    instructions.append("G1 Z20")
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E-1")
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E-1")
    instructions.append(generate_move(home_x - 60, home_y + 27))
    instructions.append(generate_move(home_x - 70, home_y + 27))
    instructions.append("G1 Z0")
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E1")
    return instructions


def backwards_brush(i, home_x, home_y):
    '''Generates gcode to perform a backwards pass through the brushes
            (for moving the yarn further down on the needle'''
    instructions = []
    instructions.append("G4 P1")
    instructions.append("L"+str(i)+" E-1")
    instructions.append(generate_tool_move(i, home_x, home_y, "brush_middle"))
    instructions += generate_latch_state_cmd(i, 0)
    instructions.append("G1 Z40")
    instructions.append(generate_tool_move(i, home_x, home_y, "brush_start"))
    instructions += generate_latch_state_cmd(i, -1)
    instructions.append("G1 Z0")
    instructions += generate_latch_state_cmd(i, -1)
    return instructions


def generate_latch_state_cmd(i, state):
    instructions = []
    instructions.append("G4 P1")
    instructions.append("L{} E{}".format(i, state))
    return instructions


def forward_brush(i, home_x, home_y):
    '''Generates gcode to perform a forwards pass through the brushes
            (for moving the yarn up, over the needle'''
    instructions = []
    instructions.append(generate_tool_move(i, home_x, home_y, "brush_start"))
    instructions += generate_latch_state_cmd(i, 1)
    instructions.append("G1 Z20")
    instructions += generate_latch_state_cmd(i, 1)
    instructions.append(generate_tool_move(i, home_x, home_y, "brush_end"))
    instructions.append("G1 Z0")
    instructions += generate_latch_state_cmd(i, 0)
    return instructions


def generate_wrap_loop(n_i, home_x, home_y, radius, theta_i, num_samples):
    '''Generates gcode to perform a wrapping loop, which is needed when moving/looping 
        across blades/rows. i.e. when the previous loop is on a needle that is immediately infront
        of or behind the current needle'''
    instructions = []
    instructions.append("T1")
    instructions.append("G1 F600")
    instructions.append(generate_move(home_x, home_y))
    instructions.append("G92 E0")
    instructions += generate_latch_state_cmd(n_i, -1)
    instructions.append("G1 Z17")
    instructions += generate_latch_state_cmd(n_i, -1)
    increment = 1
    x = 0
    y = 0
    for i in range(num_samples):
        x = home_x - radius * math.sin(math.radians(theta_i - increment * i))
        y = home_y + radius * math.cos(math.radians(theta_i - increment * i))
        instructions.append(generate_move(x, y))
    instructions.append(generate_tool_move(n_i, home_x, home_y, "clearance"))
    instructions.append("G92 E0")
    instructions.append("G1 E2")
    brush_down = forward_brush(n_i, home_x, home_y)
    instructions += brush_down
    instructions += backwards_brush(n_i, home_x, home_y)
    instructions.append("G92 E0")
    instructions.append("G1 E3")
    return instructions


def generate_rightward_loop(n_i, home_x, home_y, radius, theta_i, num_samples):
    '''Generates gcode to perform a rightward loop, which is needed when building a loop given the
    previous loop was cast on a needle to the left of the current needle'''
    instructions = []
    instructions.append("T1")
    instructions.append("G1 F600")
    instructions.append(generate_move(home_x+20, home_y))
    instructions.append("G92 E2")
    instructions.append("G1 E0")
    instructions.append("G92 E0")
    instructions.append("G4 P1")
    instructions.append("L"+str(n_i)+" E-1")
    instructions.append("G1 Z17")
    instructions.append("G92 E0")
    instructions.append(generate_move(home_x, home_y+15))
    instructions.append(generate_move(home_x - 30, home_y+15))
    instructions.append("G4 P1")
    instructions.append("L"+str(n_i)+" E1")
    instructions.append("G1 E4")
    brush_down = forward_brush(n_i, home_x, home_y)
    instructions += brush_down
    instructions += backwards_brush(n_i, home_x, home_y)
    return instructions


def generate_leftward_loop(n_i, home_x, home_y, radius, theta_i, num_samples):
    '''Generates gcode to perform a leftward loop, which is needed when building a loop given the
    previous loop was cast on a needle to the right of the current needle'''
    instructions = []
    instructions.append("T1")
    instructions.append("G1 F600")
    instructions.append(generate_move(home_x+20, home_y))
    instructions.append("G92 E2")
    instructions.append("G1 E0")
    instructions.append("G92 E0")
    instructions.append("G4 P1")
    instructions.append("L"+str(n_i)+" E-1")
    instructions.append("G1 Z17")
    instructions.append("G92 E0")
    instructions.append(generate_move(home_x, home_y-15))
    instructions.append(generate_move(home_x - 30, home_y-15))
    instructions.append("G4 P1")
    instructions.append("L"+str(n_i)+" E1")
    instructions.append("G1 E4")
    brush_down = forward_brush(n_i, home_x, home_y)
    instructions += brush_down
    instructions += backwards_brush(n_i, home_x, home_y)
    return instructions


def write_to_file(filename, instructions):
    '''Helper function to write a list of instructions to a file to be processed by repeteir'''
    with open(filename, "w+") as outfile:
        for instruction in instructions:
            outfile.write(instruction + "\n")


if __name__ == "__main__":
    for i, vals in home_positions.items():
        x, y = vals
        write_to_file("../low_level_instructions/leftward_regular_loop/n"+i+"_L_loop.txt",
                      generate_leftward_loop(i, x, y, 20, 50, 220))
        write_to_file("../low_level_instructions/wrap_loop/n"+i+"_wrap_loop.txt",
                      generate_wrap_loop(i, x, y, 20, 50, 220))
        write_to_file("../low_level_instructions/rightward_regular_loop/n"+i+"_R_loop.txt",
                      generate_rightward_loop(i, x, y, 20, 50, 220))
        write_to_file("../low_level_instructions/cast_on/n"+i +
                      "_cast_on.txt", generate_cast_on(i, x, y, 20, 50, 220))
        write_to_file("../low_level_instructions/backwards_brush/n"+i +
                      "_backwards_brush.txt", backwards_brush(i, x, y))
        write_to_file("../low_level_instructions/forward_brush/n"+i+"_forward_brush.txt", forward_brush(i, x, y))
        write_to_file("../low_level_instructions/pinch_grab/n"+i+"_pinch_grab.txt", pinch_grab(i, x, y))
