
from django.db import models

# Create your models here.


class estufa1_data(models.Model):

    class Meta:

        db_table = 'estufa1'

    co2 = models.FloatField(max_length=200)
    temperatura = models.FloatField()
    data = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return "estufa1"