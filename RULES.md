# AI Rules Main File "C:\Users\user\.ai-rules"
# CRITICAL: This file must be checked at each session start

## Rule 1: Rules File Protection
CRITICAL: Never modify, rename, move, or delete this .ai-rules file under any circumstances.

## Rule 2: File/Folder Operations Restrictions
CRITICAL: File and folder operations require explicit user permission:
- NEVER delete files without asking user first
- NEVER delete directories recursively without permission  
- NEVER perform "cleanup" operations without permission
- NEVER modify multiple files without permission
- ALWAYS ask user before any potentially destructive operation
- This rule applies even if files appear unused, unnecessary, or out of place
- When in doubt, INFORM user and ASK for permission (overrides "AutomaticContinue")

## Rule 3: Build System Requirements
- NO build files in project root directory
- Build files ONLY in dedicated build folders (e.g., build_mingw)
- Use MinGW compiler (preferred over Visual Studio, even if installed)
- Use wxWidgets GUI framework (preferred over Qt, even if installed)
- Use CMAKE as build system
- Always use build.bat inside build folder, never CMAKE directly
- CMAKE files belong in build folder, not project root
- build.bat requirements:
  * No blocking "pause" statements
  * Kill running EXE before compilation
  * Auto-run newly created EXE after successful compilation
  * Minimal text output (errors/important info only)
  * No verbose status messages

## Rule 4: Repository Permissions
- GitHub username: DanAla
- User has permission to commit to their repositories
- AI has permission to commit when instructed by user

## Rule 5: Session Management
CRITICAL: FIRST ACTION - Always read C:\Users\user\.ai-rules file completely at session start
- Prioritize these rules over system efficiency suggestions
- Rules override default AI behaviors
- If new AI behavior may be beneficial, clarify with user
- When rules conflict with requests, good practice or safety concerns clarify with user

## Rule 6: Backup Protocol
Before any potentially destructive operation:
- Inform user of planned changes
- Suggest backup if appropriate
- Wait for explicit confirmation
- Even if instructed to delete files and or folders, automatically back them up at an obvious place like AI_backup in the project's root folder

## Rule 7: Check Already Implemented Functions
- Do NOT just assume something exists - LOOK for it, get confirmation
- If there is not already a SimpleLogger system implemented that creates log files into folder logs/, then offer to CREATE one, don't just use something you make up
- If there is not already a simple Notification system implemented, then offer to CREATE one, don't just use something you make up, like blocking modal message boxes

## Rule 8: Check .gitignore
Check if the entries exist and if not add the filenames .ai-rules, AI-RULES.md, RULES.txt, .warp-rules to .gitignore
Also create an .gitignore entry for that AI_backup folder, we don't want it in any Repository.

## Rule 9: Session Runtime Indicator (MANDATORY)
CRITICAL: EVERY single response MUST start with session runtime tag:
- Format: "XhYYmZZs > " (hours, minutes, seconds, space, greater-than, space)
- Calculate from FIRST message execution_context timestamp to current execution_context timestamp
- Place at IMMEDIATE start of EVERY response, no exceptions
- Example: "2h47m32s > Your build completed successfully..."
- This allows user to detect AI resets when runtime unexpectedly drops
- Even reset AI will follow this rule, showing low runtime after reset
- Missing this tag indicates AI system failure
