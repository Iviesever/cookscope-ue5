# Resource rules

`resource.texture`, `resource.static-mesh`, `resource.skeletal-mesh`, and `resource.sound` compare normalized UE metadata with configured numeric/format limits. Missing or malformed metadata produces `MeasurementUnavailable`; it never becomes zero.
