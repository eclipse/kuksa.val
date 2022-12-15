package kuksa_client

type KuksaBackend interface {
	ConnectToKuksaVal() error
	Close()
	AuthorizeKuksaValConn() error
	GetValueFromKuksaVal(path string, attr string) ([]string, error)
	SetValueFromKuksaVal(path string, value string, attr string) error
	SubscribeFromKuksaVal(path string, attr string) (string, error)
	UnsubscribeFromKuksaVal(id string) error
	PrintSubscriptionMessages(id string) error
	GetMetadataFromKuksaVal(path string) ([]string, error)
}
