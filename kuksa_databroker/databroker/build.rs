use anyhow::Result;
use vergen::{vergen, Config};

// Extract build info (at build time)
fn main() -> Result<()> {
    let mut config = Config::default();

    // Cargo
    let cargo = config.cargo_mut();

    // Enable VERGEN_CARGO_PROFILE
    *cargo.profile_mut() = true;

    // Git
    let git = config.git_mut();

    // Rerun on HEAD change
    *git.rerun_on_head_change_mut() = true;

    // Enable VERGEN_GIT_BRANCH
    *git.branch_mut() = true;

    // Enable VERGEN_GIT_COMMIT_TIMESTAMP
    *git.commit_timestamp_mut() = true;
    *git.commit_timestamp_timezone_mut() = vergen::TimeZone::Utc;

    // Enable VERGEN_GIT_SEMVER
    *git.semver_mut() = true;
    *git.semver_kind_mut() = vergen::SemverKind::Normal;
    *git.semver_dirty_mut() = Some(" (dirty worktree)");

    // Enable VERGEN_GIT_SHA
    *git.sha_mut() = true;

    match vergen(config) {
        Ok(ok) => Ok(ok),
        Err(e) => {
            // Swallow the errors for now (enable with -vv)
            eprintln!("vergen failed: {}", e);
            Ok(())
        }
    }
}
