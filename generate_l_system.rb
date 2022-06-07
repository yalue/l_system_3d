symbols = ['S']

# Generates a replacement rule. Returns ["rule string", [new used_symbols]]
def random_replacement_rule(all_symbols, used_symbols, rule_symbols)
  # We can't redefine rule symbols.
  return ["", used_symbols, rule_symbols] if used_symbols.length == rule_symbols.length
  start = used_symbols.sample()
  start = used_symbols.sample() while rule_symbols.include?(start)
  rule_symbols << start
  replacement_size = [2, 3, 3, 3, 4, 5].sample()
  replacement = ""
  replacement_size.times do
    # Only choose a new symbol 1/3rd of the time. (Tehcnically less, since all
    # symbols will include the used ones.
    if (used_symbols.length < 3) || (rand() < 0.3)
      symbol = all_symbols.sample()
      used_symbols << symbol if !used_symbols.include?(symbol)
    else
      symbol = used_symbols.sample()
    end
    if rand() < 0.01
      # 1% of the time do a push & pop
      replacement += "(" + symbol + ")"
    else
      replacement += symbol
    end
  end
  rule_string = start + " " + replacement
  [rule_string, used_symbols, rule_symbols]
end

def get_move_action()
  "move_forward %.03f" % [rand()]
end

def get_rotate_action()
  action = ["rotate", "pitch", "roll"].sample()
  angle = 180.0 - (360.0 * rand())
  "%s %.03f" % [action, angle]
end

def get_setcolor_action()
  channel = ["r", "g", "b"].sample()
  "set_color_%s %.03f" % [channel, rand()]
end

# Returns an array of random action strings, NOT including push and pop.
def generate_random_actions()
  moves = ["move"] * 6
  rotates = ["rotate"] * 4
  set_colors = ["set_color"] * 2
  action_types = moves + rotates + set_colors
  actions_not_move = rotates + set_colors
  to_return = []
  action_count = rand(8)
  prev_was_move = false
  action_count.times do
    if prev_was_move
      action_type = actions_not_move.sample()
    else
      action_type = action_types.sample()
    end
    prev_was_move = false
    if action_type == "move"
      prev_was_move = true
      to_return << get_move_action()
    elsif action_type == "rotate"
      to_return << get_rotate_action()
    elsif action_type == "set_color"
      to_return << get_setcolor_action()
    end
  end
  to_return
end

# Don't include the start symbol in all_symbols
all_symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz".chars
used_symbols = ["S"]
rule_symbols = []
puts "init S"
10.times do
  tmp = random_replacement_rule(all_symbols, used_symbols, rule_symbols)
  puts tmp[0]
  used_symbols = tmp[1]
  rule_symbols = tmp[2]
end

puts "\nactions
(
push_color 0.0
push_position 0.0

)
pop_color 0.0
pop_position 0.0

"

used_symbols.each do |s|
  # Only generate actions for about 1/3rd of the symbols
  next if rand() < 0.3
  puts "#{s}"
  actions = generate_random_actions()
  actions.each {|a| puts a}
  puts ""
end

