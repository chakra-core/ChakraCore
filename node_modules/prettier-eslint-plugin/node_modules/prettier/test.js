// https://github.com/prettier/prettier/issues/959
Seq(typeDef.interface.groups).forEach(group =>
  Seq(group.members).forEach((member, memberName) =>
    markdownDoc(
      member.doc,
      { typePath: typePath.concat(memberName.slice(1)),
       signatures: member.signatures }
    )
  )
)

// https://github.com/prettier/prettier/issues/760
const promiseFromCallback = fn => 
  new Promise((resolve, reject) => 
    fn((err, result) => {
      if (err) return reject(err);
      return resolve(result);
    })
  );

// https://github.com/prettier/prettier/pull/674
runtimeAgent.getProperties(
  objectId,
  false, // ownProperties
  false, // accessorPropertiesOnly
  false, // generatePreview
  (error, properties, internalProperties) => {
    return 1
  },
);

// https://github.com/prettier/prettier/pull/680
function render() {
  return (
    <View>
      <Image
        onProgress={(e) => this.setState({progress: Math.round(100 * e.nativeEvent.loaded / e.nativeEvent.total)})}
      />
    </View>
  );
}

function render() {
  return (
    <View>
      <Image
        onProgress={e =>
          this.setState({
            progress: Math.round(
              100 * e.nativeEvent.loaded / e.nativeEvent.total,
            ),
          })}
      />
    </View>
  );
}

function render() {
  return (
    <View>
      <Image
        onProgress={e =>
          this.setState({
            progress: Math.round(
              100 * e.nativeEvent.loaded / e.nativeEvent.total,
            ),
          })}
      />
    </View>
  );
}