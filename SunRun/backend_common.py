class DataPoint:
    def __init__(self, generated_ts, data):
        self.generated_ts = generated_ts
        self.data = data

    def __repr__(self):
        return "DataPoint(generated_ts={}, data={})".format(self.generated_ts, self.data)
