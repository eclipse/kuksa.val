use anyhow::Result;
use vergen::{vergen, Config};

// Extract build info (at build time)
fn main() -> Result<()> {
    let mut config = Config::default();

    // Cargo
    let cargo = config.cargo_mut();
    *cargo.features_mut() = false;
    // Enable VERGEN_CARGO_PROFILE
    *cargo.profile_mut() = true;
    *cargo.target_triple_mut() = false;

    // Git
    let git = config.git_mut();
    *git.commit_author_mut() = false;
    *git.commit_count_mut() = false;
    *git.commit_message_mut() = false;

    // Rerun on HEAD change
    *git.rerun_on_head_change_mut() = true;

    // Enable VERGEN_GIT_BRANCH
    *git.branch_mut() = true;

    // Enable VERGEN_GIT_COMMIT_TIMESTAMP
    *git.commit_timestamp_mut() = true;
    *git.commit_timestamp_timezone_mut() = vergen::TimeZone::Utc;

    // Enable VERGEN_GIT_SEMVER
    *git.semver_mut() = true;
    *git.semver_kind_mut() = vergen::SemverKind::Lightweight;
    *git.semver_dirty_mut() = Some(" (dirty worktree)");

    // Enable VERGEN_GIT_SHA
    *git.sha_mut() = true;

    match vergen(config) {
        Ok(ok) => Ok(ok),
        Err(e) => {
            // Swallow the errors for now (enable with -vv)
            eprintln!("vergen failed: {e}");
            Ok(())
        }
    }
}
