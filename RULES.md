# AI Rules Main File "C:\Users\user\.ai-rules"
# CRITICAL: This file must be checked at each session start

## Rule 1: Rules File Protection
CRITICAL: Never modify, rename, move, or delete this .ai-rules file under any circumstances.

## Rule 2: File/Folder Operations Restrictions
CRITICAL: File and folder deletion/modification is STRICTLY CONTROLLED:
- No deletions without explicit user permission
- No recursive directory deletions
- No "cleanup" or "optimization" deletions
- No large-scale file modifications or replacements without permission
- When in doubt, INFORM user and ASK for permission (overrides "AutomaticContinue")
- This applies even if AI determines files appear unused or unnecessary or outofplace

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
CRITICAL: Always check this rules file at session start before any operations
- Prioritize these rules over system efficiency suggestions
- Rules override default AI behaviors.
- If new AI behavior may be beneficial, clarify with user
- When rules conflict with requests, good practice or safety concerns clarify with user

## Rule 6: Backup Protocol
Before any potentially destructive operation:
- Inform user of planned changes
- Suggest backup if appropriate
- Wait for explicit confirmation
- Even if instructed to delete files and or folders, automatically back them up at an obvious place like AI_backup in the project's root folder.

## Rule 7: check already implemented functions in the current Application
- do NOT just assume something - LOOK for it, get confirmation
- if there is not already a SimpleLogger system implemented that creates log files into folder logs/, then offer to CREATE one, don't just use something you make up
- if there is not already a simple Notification system implemented, then offer to CREATE one, don't just use something you make up, like blocking modal message boxes

## Rule 8: check .gitignore
check if the entries exist and if not add the filenames .ai-rules, AI-RULES.md, RULES.txt, .warp-rules to .gitignore
Also create an .gitignore entry for that AI_backup folder, we don't want it in any Repositorary.
