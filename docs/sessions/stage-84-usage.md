# stage 84 usage

Session

  Total cost:            $1.00
  Total duration (API):  4m 24s
  Total duration (wall): 17m 16s
  Total code changes:    138 lines added, 13 lines removed
  Usage by model:             
     claude-sonnet-4-6:  393 input, 11.7k output, 2.0m cache read, 42.7k cache write ($0.94)
      claude-haiku-4-5:  39 input, 3.1k output, 52.5k cache read, 25.9k cache write ($0.0533)
                              
  Current session
  ███████████▌                                       23% used
  Resets 11:20pm (America/New_York)
                              
  Current week (all models)
  ███████████████████                                38% used
  Resets Jun 5, 10am (America/New_York)

  What's contributing to your limits usage?
  Approximate, based on local sessions on this machine — does not include other devices or claude.ai

  Last 24h · these are independent characteristics of your usage, not a breakdown

  86% of your usage came from subagent-heavy sessions
   Each subagent runs its own requests. Be deliberate about spawning them — and 
   consider configuring a cheaper model for simpler subagents.

  15% of your usage was at >150k context
   Longer sessions are more expensive even when cached. /compact mid-task, /clear
   when switching to new tasks.

  56% of your usage came from /implement-stage
   Heavy skills can be scoped down or run with a cheaper model via skill
   frontmatter.

  Skills                  % of usage
  /implement-stage               56%
  /self-compile-report           11%

  Subagents               % of usage
  implement-stage                 1%

