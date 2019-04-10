const SHOULD_INCLUDE_HELPERS = new WeakMap();

function shouldIncludeHelpers(addonOrProject) {
  // Currently we check for @ember-decorators transforms specifically, but we
  // could check for any number of heuristics in this function. What we want is
  // to default to on if we reasonably believe that users will incur massive
  // cost for inlining helpers. Stage 2+ decorators are a very clear indicator
  // that helpers should be included, at 12kb for the helpers, it pays for
  // itself after usage in 5 files. With stage 1 decorators, it pays for itself
  // after 25 files.
  if (addonOrProject.pkg && addonOrProject.pkg.name === '@ember-decorators/babel-transforms') {
    return true;
  }

  if (addonOrProject.addons) {
    return addonOrProject.addons.some(shouldIncludeHelpers);
  }

  return false;
}


module.exports = function defaultShouldIncludeHelpers(project) {
  if (SHOULD_INCLUDE_HELPERS.has(project)) {
    return SHOULD_INCLUDE_HELPERS.get(project);
  }

  let shouldInclude = shouldIncludeHelpers(project);

  SHOULD_INCLUDE_HELPERS.set(project, shouldInclude);

  return shouldInclude;
}

